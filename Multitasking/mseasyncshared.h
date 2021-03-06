
// Copyright (c) 2015 Noah Lopez
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef MSEASYNCSHARED_H_
#define MSEASYNCSHARED_H_

#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <cassert>


#if defined(MSE_SAFER_SUBSTITUTES_DISABLED) || defined(MSE_SAFERPTR_DISABLED)
#define MSE_ASYNCSHAREDPOINTER_DISABLED
#endif /*defined(MSE_SAFER_SUBSTITUTES_DISABLED) || defined(MSE_SAFERPTR_DISABLED)*/

namespace mse {

#ifdef MSE_ASYNCSHAREDPOINTER_DISABLED
#else /*MSE_ASYNCSHAREDPOINTER_DISABLED*/
#endif /*MSE_ASYNCSHAREDPOINTER_DISABLED*/

	/* This macro roughly simulates constructor inheritance. Originally it was used when some compilers didn't support
	constructor inheritance, but now we use it because of it's differences with standard constructor inheritance. */
#define MSE_ASYNC_USING(Derived, Base) \
    template<typename ...Args, typename = typename std::enable_if<std::is_constructible<Base, Args...>::value>::type> \
    Derived(Args &&...args) : Base(std::forward<Args>(args)...) {}

	template <class _Ty>
	class unlock_guard {
	public:
		unlock_guard(_Ty& mutex_ref) : m_mutex_ref(mutex_ref) {
			m_mutex_ref.unlock();
		}
		~unlock_guard() {
			m_mutex_ref.lock();
		}

		_Ty& m_mutex_ref;
	};

	class rstm_bad_alloc : public std::bad_alloc {
	public:
		rstm_bad_alloc(const std::string& what) : m_what(what) {}
		virtual const char* what() const noexcept { return m_what.c_str(); }
		std::string m_what;
	};

	class recursive_shared_timed_mutex : private std::shared_timed_mutex {
	public:
		typedef std::shared_timed_mutex base_class;

		void lock()
		{	// lock exclusive
			std::lock_guard<std::mutex> lock1(m_write_mutex);

			if ((1 <= m_writelock_count) && (std::this_thread::get_id() == m_writelock_thread_id)) {
			}
			else {
				assert((std::this_thread::get_id() != m_writelock_thread_id) || (0 == m_writelock_count));
				{
					unlock_guard<std::mutex> unlock1(m_write_mutex);
					base_class::lock();
				}
				m_writelock_thread_id = std::this_thread::get_id();
				assert(0 == m_writelock_count);
			}
			m_writelock_count += 1;
		}

		bool try_lock()
		{	// try to lock exclusive
			bool retval = false;
			std::lock_guard<std::mutex> lock1(m_write_mutex);

			if ((1 <= m_writelock_count) && (std::this_thread::get_id() == m_writelock_thread_id)) {
				m_writelock_count += 1;
				retval = true;
			}
			else {
				assert(0 == m_writelock_count);
				retval = base_class::try_lock();
				if (retval) {
					m_writelock_thread_id = std::this_thread::get_id();
					m_writelock_count += 1;
				}
			}
			return retval;
		}

		template<class _Rep, class _Period>
		bool try_lock_for(const std::chrono::duration<_Rep, _Period>& _Rel_time)
		{	// try to lock for duration
			return (try_lock_until(std::chrono::steady_clock::now() + _Rel_time));
		}

		template<class _Clock, class _Duration>
		bool try_lock_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time)
		{	// try to lock until time point
			bool retval = false;
			std::lock_guard<std::mutex> lock1(m_write_mutex);

			if ((1 <= m_writelock_count) && (std::this_thread::get_id() == m_writelock_thread_id)) {
				m_writelock_count += 1;
				retval = true;
			}
			else {
				assert(0 == m_writelock_count);
				retval = base_class::try_lock_until(_Abs_time);
				if (retval) {
					m_writelock_thread_id = std::this_thread::get_id();
					m_writelock_count += 1;
				}
			}
			return retval;
		}

		void unlock()
		{	// unlock exclusive
			std::lock_guard<std::mutex> lock1(m_write_mutex);
			assert(std::this_thread::get_id() == m_writelock_thread_id);

			if ((2 <= m_writelock_count) && (std::this_thread::get_id() == m_writelock_thread_id)) {
			}
			else {
				assert(1 == m_writelock_count);
				base_class::unlock();
			}
			m_writelock_count -= 1;
		}

		void lock_shared()
		{	// lock non-exclusive
			std::lock_guard<std::mutex> lock1(m_read_mutex);

			const auto this_thread_id = std::this_thread::get_id();
			const auto found_it = m_thread_id_readlock_count_map.find(this_thread_id);
			if ((m_thread_id_readlock_count_map.end() != found_it) && (1 <= (*found_it).second)) {
				(*found_it).second += 1;
			}
			else {
				assert((m_thread_id_readlock_count_map.end() == found_it) || (0 == (*found_it).second));
				{
					unlock_guard<std::mutex> unlock1(m_read_mutex);
					base_class::lock_shared();
				}
				try {
					/* Things could've changed so we have to check again. */
					const auto l_found_it = m_thread_id_readlock_count_map.find(this_thread_id);
					if (m_thread_id_readlock_count_map.end() != l_found_it) {
						assert(0 <= (*l_found_it).second);
						(*l_found_it).second += 1;
					}
					else {
							std::unordered_map<std::thread::id, int>::value_type item(this_thread_id, 1);
							m_thread_id_readlock_count_map.insert(item);
					}
				}
				catch (...) {
					base_class::unlock_shared();
					throw(rstm_bad_alloc("std::unordered_map<>::insert() failed? - mse::recursive_shared_timed_mutex"));
				}
			}
		}

		bool try_lock_shared()
		{	// try to lock non-exclusive
			bool retval = false;
			std::lock_guard<std::mutex> lock1(m_read_mutex);

			const auto this_thread_id = std::this_thread::get_id();
			const auto found_it = m_thread_id_readlock_count_map.find(this_thread_id);
			if ((m_thread_id_readlock_count_map.end() != found_it) && (1 <= (*found_it).second)) {
				(*found_it).second += 1;
				retval = true;
			}
			else {
				retval = base_class::try_lock_shared();
				if (retval) {
					try {
						if (m_thread_id_readlock_count_map.end() != found_it) {
							assert(0 <= (*found_it).second);
							(*found_it).second += 1;
						}
						else {
							std::unordered_map<std::thread::id, int>::value_type item(this_thread_id, 1);
							m_thread_id_readlock_count_map.insert(item);
						}
					}
					catch (...) {
						base_class::unlock_shared();
						throw(rstm_bad_alloc("std::unordered_map<>::insert() failed? - mse::recursive_shared_timed_mutex"));
					}
				}
			}
			return retval;
		}

		template<class _Rep, class _Period>
		bool try_lock_shared_for(const std::chrono::duration<_Rep, _Period>& _Rel_time)
		{	// try to lock non-exclusive for relative time
			return (try_lock_shared_until(_Rel_time + std::chrono::steady_clock::now()));
		}

		template<class _Clock, class _Duration>
		bool try_lock_shared_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time)
		{	// try to lock non-exclusive until absolute time
			bool retval = false;
			std::lock_guard<std::mutex> lock1(m_read_mutex);

			const auto this_thread_id = std::this_thread::get_id();
			const auto found_it = m_thread_id_readlock_count_map.find(this_thread_id);
			if ((m_thread_id_readlock_count_map.end() != found_it) && (1 <= (*found_it).second)) {
				(*found_it).second += 1;
				retval = true;
			}
			else {
				retval = base_class::try_lock_shared_until(_Abs_time);
				if (retval) {
					try {
						if (m_thread_id_readlock_count_map.end() != found_it) {
							assert(0 <= (*found_it).second);
							(*found_it).second += 1;
						}
						else {
							std::unordered_map<std::thread::id, int>::value_type item(this_thread_id, 1);
							m_thread_id_readlock_count_map.insert(item);
						}
					}
					catch (...) {
						base_class::unlock_shared();
						throw(rstm_bad_alloc("std::unordered_map<>::insert() failed? - mse::recursive_shared_timed_mutex"));
					}
				}
			}
			return retval;
		}

		void unlock_shared()
		{	// unlock non-exclusive
			std::lock_guard<std::mutex> lock1(m_read_mutex);

			const auto this_thread_id = std::this_thread::get_id();
			const auto found_it = m_thread_id_readlock_count_map.find(this_thread_id);
			if (m_thread_id_readlock_count_map.end() != found_it) {
				if (2 <= (*found_it).second) {
					(*found_it).second -= 1;
				}
				else {
					assert(1 == (*found_it).second);
					m_thread_id_readlock_count_map.erase(found_it);
					base_class::unlock_shared();
				}
			}
			else {
				assert(false);
				base_class::unlock_shared();
			}
		}

		std::mutex m_write_mutex;
		std::mutex m_read_mutex;

		std::thread::id m_writelock_thread_id;
		int m_writelock_count = 0;
		std::unordered_map<std::thread::id, int> m_thread_id_readlock_count_map;
	};

	//typedef std::shared_timed_mutex async_shared_timed_mutex_type;
	typedef recursive_shared_timed_mutex async_shared_timed_mutex_type;

	template<typename _Ty> class TAsyncSharedReadWriteAccessRequester;
	template<typename _Ty> class TAsyncSharedReadWritePointer;
	template<typename _Ty> class TAsyncSharedReadWriteConstPointer;
	template<typename _Ty> class TAsyncSharedReadOnlyAccessRequester;
	template<typename _Ty> class TAsyncSharedReadOnlyConstPointer;

	template<typename _Ty> class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteAccessRequester;
	template<typename _Ty> class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer;
	template<typename _Ty> class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer;
	template<typename _Ty> class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyAccessRequester;
	template<typename _Ty> class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer;

	/* TAsyncSharedObj is intended as a transparent wrapper for other classes/objects. */
	template<typename _TROy>
	class TAsyncSharedObj : public _TROy {
	public:
		virtual ~TAsyncSharedObj() {}
		using _TROy::operator=;
		//TAsyncSharedObj& operator=(TAsyncSharedObj&& _X) { _TROy::operator=(std::move(_X)); return (*this); }
		TAsyncSharedObj& operator=(typename std::conditional<std::is_const<_TROy>::value
			, std::nullptr_t, TAsyncSharedObj>::type&& _X) { _TROy::operator=(std::move(_X)); return (*this); }
		//TAsyncSharedObj& operator=(const TAsyncSharedObj& _X) { _TROy::operator=(_X); return (*this); }
		TAsyncSharedObj& operator=(const typename std::conditional<std::is_const<_TROy>::value
			, std::nullptr_t, TAsyncSharedObj>::type& _X) { _TROy::operator=(_X); return (*this); }

	private:
		MSE_ASYNC_USING(TAsyncSharedObj, _TROy);
		TAsyncSharedObj(const TAsyncSharedObj& _X) : _TROy(_X) {}
		TAsyncSharedObj(TAsyncSharedObj&& _X) : _TROy(std::move(_X)) {}
		TAsyncSharedObj* operator&() {
			return this;
		}
		const TAsyncSharedObj* operator&() const {
			return this;
		}

		mutable async_shared_timed_mutex_type m_mutex1;

		friend class TAsyncSharedReadWriteAccessRequester<_TROy>;
		friend class TAsyncSharedReadWritePointer<_TROy>;
		friend class TAsyncSharedReadWriteConstPointer<_TROy>;
		friend class TAsyncSharedReadOnlyAccessRequester<_TROy>;
		friend class TAsyncSharedReadOnlyConstPointer<_TROy>;

		friend class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteAccessRequester<_TROy>;
		friend class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer<_TROy>;
		friend class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer<_TROy>;
		friend class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyAccessRequester<_TROy>;
		friend class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer<_TROy>;
	};

	template<typename _Ty>
	class TAsyncSharedReadWritePointer {
	public:
		TAsyncSharedReadWritePointer(TAsyncSharedReadWritePointer&& src) = default;
		virtual ~TAsyncSharedReadWritePointer() {}

		operator bool() const {
			//if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedReadWritePointer")); }
			return m_shptr.operator bool();
		}
		typename std::conditional<std::is_const<_Ty>::value
			, const TAsyncSharedObj<_Ty>&, TAsyncSharedObj<_Ty>&>::type operator*() const {
			if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedReadWritePointer")); }
			return (*m_shptr);
		}
		typename std::conditional<std::is_const<_Ty>::value
			, const TAsyncSharedObj<_Ty>*, TAsyncSharedObj<_Ty>*>::type operator->() const {
			if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedReadWritePointer")); }
			return std::addressof(*m_shptr);
		}
	private:
		TAsyncSharedReadWritePointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr) : m_shptr(shptr), m_unique_lock(shptr->m_mutex1) {}
		TAsyncSharedReadWritePointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t) : m_shptr(shptr), m_unique_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock()) {
				shptr = nullptr;
			}
		}
		template<class _Rep, class _Period>
		TAsyncSharedReadWritePointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t, const std::chrono::duration<_Rep, _Period>& _Rel_time) : m_shptr(shptr), m_unique_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock_for(_Rel_time)) {
				shptr = nullptr;
			}
		}
		template<class _Clock, class _Duration>
		TAsyncSharedReadWritePointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t, const std::chrono::time_point<_Clock, _Duration>& _Abs_time) : m_shptr(shptr), m_unique_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock_until(_Abs_time)) {
				shptr = nullptr;
			}
		}
		TAsyncSharedReadWritePointer<_Ty>& operator=(const TAsyncSharedReadWritePointer<_Ty>& _Right_cref) = delete;
		TAsyncSharedReadWritePointer<_Ty>& operator=(TAsyncSharedReadWritePointer<_Ty>&& _Right) = delete;

		TAsyncSharedReadWritePointer<_Ty>* operator&() { return this; }
		const TAsyncSharedReadWritePointer<_Ty>* operator&() const { return this; }
		bool is_valid() const {
			bool retval = m_shptr.operator bool();
			return retval;
		}

		std::shared_ptr<TAsyncSharedObj<_Ty>> m_shptr;
		std::unique_lock<async_shared_timed_mutex_type> m_unique_lock;

		friend class TAsyncSharedReadWriteAccessRequester<_Ty>;
	};

	template<typename _Ty>
	class TAsyncSharedReadWriteConstPointer {
	public:
		TAsyncSharedReadWriteConstPointer(TAsyncSharedReadWriteConstPointer&& src) = default;
		virtual ~TAsyncSharedReadWriteConstPointer() {}

		operator bool() const {
			//if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedReadWriteConstPointer")); }
			return m_shptr.operator bool();
		}
		const TAsyncSharedObj<const _Ty>& operator*() const {
			if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedReadWriteConstPointer")); }
			const TAsyncSharedObj<const _Ty>* extra_const_ptr = reinterpret_cast<const TAsyncSharedObj<const _Ty>*>(std::addressof(*m_shptr));
			return (*extra_const_ptr);
		}
		const TAsyncSharedObj<const _Ty>* operator->() const {
			if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedReadWriteConstPointer")); }
			const TAsyncSharedObj<const _Ty>* extra_const_ptr = reinterpret_cast<const TAsyncSharedObj<const _Ty>*>(std::addressof(*m_shptr));
			return extra_const_ptr;
		}
	private:
		TAsyncSharedReadWriteConstPointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr) : m_shptr(shptr), m_unique_lock(shptr->m_mutex1) {}
		TAsyncSharedReadWriteConstPointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t) : m_shptr(shptr), m_unique_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock()) {
				shptr = nullptr;
			}
		}
		template<class _Rep, class _Period>
		TAsyncSharedReadWriteConstPointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t, const std::chrono::duration<_Rep, _Period>& _Rel_time) : m_shptr(shptr), m_unique_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock_for(_Rel_time)) {
				shptr = nullptr;
			}
		}
		template<class _Clock, class _Duration>
		TAsyncSharedReadWriteConstPointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t, const std::chrono::time_point<_Clock, _Duration>& _Abs_time) : m_shptr(shptr), m_unique_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock_until(_Abs_time)) {
				shptr = nullptr;
			}
		}
		TAsyncSharedReadWriteConstPointer<_Ty>& operator=(const TAsyncSharedReadWriteConstPointer<_Ty>& _Right_cref) = delete;
		TAsyncSharedReadWriteConstPointer<_Ty>& operator=(TAsyncSharedReadWriteConstPointer<_Ty>&& _Right) = delete;

		TAsyncSharedReadWriteConstPointer<_Ty>* operator&() { return this; }
		const TAsyncSharedReadWriteConstPointer<_Ty>* operator&() const { return this; }
		bool is_valid() const {
			bool retval = m_shptr.operator bool();
			return retval;
		}

		std::shared_ptr<TAsyncSharedObj<_Ty>> m_shptr;
		std::unique_lock<async_shared_timed_mutex_type> m_unique_lock;

		friend class TAsyncSharedReadWriteAccessRequester<_Ty>;
	};

	template<typename _Ty>
	class TAsyncSharedReadWriteAccessRequester {
	public:
		TAsyncSharedReadWriteAccessRequester(const TAsyncSharedReadWriteAccessRequester& src_cref) = default;

		TAsyncSharedReadWritePointer<_Ty> writelock_ptr() {
			return TAsyncSharedReadWritePointer<_Ty>(m_shptr);
		}
		TAsyncSharedReadWritePointer<_Ty> try_writelock_ptr() {
			return TAsyncSharedReadWritePointer<_Ty>(m_shptr, std::try_to_lock);
		}
		template<class _Rep, class _Period>
		TAsyncSharedReadWritePointer<_Ty> try_writelock_ptr_for(const std::chrono::duration<_Rep, _Period>& _Rel_time) {
			return TAsyncSharedReadWritePointer<_Ty>(m_shptr, std::try_to_lock, _Rel_time);
		}
		template<class _Clock, class _Duration>
		TAsyncSharedReadWritePointer<_Ty> try_writelock_ptr_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time) {
			return TAsyncSharedReadWritePointer<_Ty>(m_shptr, std::try_to_lock, _Abs_time);
		}
		TAsyncSharedReadWriteConstPointer<_Ty> readlock_ptr() {
			return TAsyncSharedReadWriteConstPointer<_Ty>(m_shptr);
		}
		TAsyncSharedReadWriteConstPointer<_Ty> try_readlock_ptr() {
			return TAsyncSharedReadWriteConstPointer<_Ty>(m_shptr, std::try_to_lock);
		}
		template<class _Rep, class _Period>
		TAsyncSharedReadWriteConstPointer<_Ty> try_readlock_ptr_for(const std::chrono::duration<_Rep, _Period>& _Rel_time) {
			return TAsyncSharedReadWriteConstPointer<_Ty>(m_shptr, std::try_to_lock, _Rel_time);
		}
		template<class _Clock, class _Duration>
		TAsyncSharedReadWriteConstPointer<_Ty> try_readlock_ptr_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time) {
			return TAsyncSharedReadWriteConstPointer<_Ty>(m_shptr, std::try_to_lock, _Abs_time);
		}

		template <class... Args>
		static TAsyncSharedReadWriteAccessRequester make_asyncsharedreadwrite(Args&&... args) {
			//auto shptr = std::make_shared<TAsyncSharedObj<_Ty>>(std::forward<Args>(args)...);
			std::shared_ptr<TAsyncSharedObj<_Ty>> shptr(new TAsyncSharedObj<_Ty>(std::forward<Args>(args)...));
			TAsyncSharedReadWriteAccessRequester retval(shptr);
			return retval;
		}

	private:
		TAsyncSharedReadWriteAccessRequester(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr) : m_shptr(shptr) {}

		TAsyncSharedReadWriteAccessRequester<_Ty>* operator&() { return this; }
		const TAsyncSharedReadWriteAccessRequester<_Ty>* operator&() const { return this; }

		std::shared_ptr<TAsyncSharedObj<_Ty>> m_shptr;

		friend class TAsyncSharedReadOnlyAccessRequester<_Ty>;
	};

	template <class X, class... Args>
	TAsyncSharedReadWriteAccessRequester<X> make_asyncsharedreadwrite(Args&&... args) {
		return TAsyncSharedReadWriteAccessRequester<X>::make_asyncsharedreadwrite(std::forward<Args>(args)...);
	}


	template<typename _Ty>
	class TAsyncSharedReadOnlyConstPointer {
	public:
		TAsyncSharedReadOnlyConstPointer(TAsyncSharedReadOnlyConstPointer&& src) = default;
		virtual ~TAsyncSharedReadOnlyConstPointer() {}

		operator bool() const {
			//if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedReadOnlyConstPointer")); }
			return m_shptr.operator bool();
		}
		const TAsyncSharedObj<const _Ty>& operator*() const {
			if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedReadOnlyConstPointer")); }
			const TAsyncSharedObj<const _Ty>* extra_const_ptr = reinterpret_cast<const TAsyncSharedObj<const _Ty>*>(std::addressof(*m_shptr));
			return (*extra_const_ptr);
		}
		const TAsyncSharedObj<const _Ty>* operator->() const {
			if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedReadOnlyConstPointer")); }
			const TAsyncSharedObj<const _Ty>* extra_const_ptr = reinterpret_cast<const TAsyncSharedObj<const _Ty>*>(std::addressof(*m_shptr));
			return extra_const_ptr;
		}
	private:
		TAsyncSharedReadOnlyConstPointer(std::shared_ptr<const TAsyncSharedObj<_Ty>> shptr) : m_shptr(shptr), m_unique_lock(shptr->m_mutex1) {}
		TAsyncSharedReadOnlyConstPointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t) : m_shptr(shptr), m_unique_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock()) {
				shptr = nullptr;
			}
		}
		template<class _Rep, class _Period>
		TAsyncSharedReadOnlyConstPointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t, const std::chrono::duration<_Rep, _Period>& _Rel_time) : m_shptr(shptr), m_unique_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock_for(_Rel_time)) {
				shptr = nullptr;
			}
		}
		template<class _Clock, class _Duration>
		TAsyncSharedReadOnlyConstPointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t, const std::chrono::time_point<_Clock, _Duration>& _Abs_time) : m_shptr(shptr), m_unique_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock_until(_Abs_time)) {
				shptr = nullptr;
			}
		}
		TAsyncSharedReadOnlyConstPointer<_Ty>& operator=(const TAsyncSharedReadOnlyConstPointer<_Ty>& _Right_cref) = delete;
		TAsyncSharedReadOnlyConstPointer<_Ty>& operator=(TAsyncSharedReadOnlyConstPointer<_Ty>&& _Right) = delete;

		TAsyncSharedReadOnlyConstPointer<_Ty>* operator&() { return this; }
		const TAsyncSharedReadOnlyConstPointer<_Ty>* operator&() const { return this; }
		bool is_valid() const {
			bool retval = m_shptr.operator bool();
			return retval;
		}

		std::shared_ptr<const TAsyncSharedObj<_Ty>> m_shptr;
		std::unique_lock<async_shared_timed_mutex_type> m_unique_lock;

		friend class TAsyncSharedReadOnlyAccessRequester<_Ty>;
	};

	template<typename _Ty>
	class TAsyncSharedReadOnlyAccessRequester {
	public:
		TAsyncSharedReadOnlyAccessRequester(const TAsyncSharedReadOnlyAccessRequester& src_cref) = default;
		TAsyncSharedReadOnlyAccessRequester(const TAsyncSharedReadWriteAccessRequester<_Ty>& src_cref) : m_shptr(src_cref.m_shptr) {}

		TAsyncSharedReadOnlyConstPointer<_Ty> readlock_ptr() {
			return TAsyncSharedReadOnlyConstPointer<_Ty>(m_shptr);
		}
		TAsyncSharedReadOnlyConstPointer<_Ty> try_readlock_ptr() {
			return TAsyncSharedReadOnlyConstPointer<_Ty>(m_shptr, std::try_to_lock);
		}
		template<class _Rep, class _Period>
		TAsyncSharedReadOnlyConstPointer<_Ty> try_readlock_ptr_for(const std::chrono::duration<_Rep, _Period>& _Rel_time) {
			return TAsyncSharedReadOnlyConstPointer<_Ty>(m_shptr, std::try_to_lock, _Rel_time);
		}
		template<class _Clock, class _Duration>
		TAsyncSharedReadOnlyConstPointer<_Ty> try_readlock_ptr_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time) {
			return TAsyncSharedReadOnlyConstPointer<_Ty>(m_shptr, std::try_to_lock, _Abs_time);
		}

		template <class... Args>
		static TAsyncSharedReadOnlyAccessRequester make_asyncsharedreadonly(Args&&... args) {
			//auto shptr = std::make_shared<const TAsyncSharedObj<_Ty>>(std::forward<Args>(args)...);
			std::shared_ptr<const TAsyncSharedObj<_Ty>> shptr(new const TAsyncSharedObj<_Ty>(std::forward<Args>(args)...));
			TAsyncSharedReadOnlyAccessRequester retval(shptr);
			return retval;
		}

	private:
		TAsyncSharedReadOnlyAccessRequester(std::shared_ptr<const TAsyncSharedObj<_Ty>> shptr) : m_shptr(shptr) {}

		TAsyncSharedReadOnlyAccessRequester<_Ty>* operator&() { return this; }
		const TAsyncSharedReadOnlyAccessRequester<_Ty>* operator&() const { return this; }

		std::shared_ptr<const TAsyncSharedObj<_Ty>> m_shptr;
	};

	template <class X, class... Args>
	TAsyncSharedReadOnlyAccessRequester<X> make_asyncsharedreadonly(Args&&... args) {
		return TAsyncSharedReadOnlyAccessRequester<X>::make_asyncsharedreadonly(std::forward<Args>(args)...);
	}


	template<typename _Ty>
	class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer {
	public:
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer(TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer&& src) = default;
		virtual ~TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer() {}

		operator bool() const {
			//if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer")); }
			return m_shptr.operator bool();
		}
		typename std::conditional<std::is_const<_Ty>::value
			, const TAsyncSharedObj<_Ty>&, TAsyncSharedObj<_Ty>&>::type operator*() const {
			if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer")); }
			return (*m_shptr);
		}
		typename std::conditional<std::is_const<_Ty>::value
			, const TAsyncSharedObj<_Ty>*, TAsyncSharedObj<_Ty>*>::type operator->() const {
			if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer")); }
			return std::addressof(*m_shptr);
		}
	private:
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr) : m_shptr(shptr), m_unique_lock(shptr->m_mutex1) {}
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t) : m_shptr(shptr), m_unique_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock()) {
				shptr = nullptr;
			}
		}
		template<class _Rep, class _Period>
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t, const std::chrono::duration<_Rep, _Period>& _Rel_time) : m_shptr(shptr), m_unique_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock_for(_Rel_time)) {
				shptr = nullptr;
			}
		}
		template<class _Clock, class _Duration>
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t, const std::chrono::time_point<_Clock, _Duration>& _Abs_time) : m_shptr(shptr), m_unique_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock_until(_Abs_time)) {
				shptr = nullptr;
			}
		}
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer<_Ty>& operator=(const TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer<_Ty>& _Right_cref) = delete;
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer<_Ty>& operator=(TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer<_Ty>&& _Right) = delete;

		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer<_Ty>* operator&() { return this; }
		const TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer<_Ty>* operator&() const { return this; }
		bool is_valid() const {
			bool retval = m_shptr.operator bool();
			return retval;
		}

		std::shared_ptr<TAsyncSharedObj<_Ty>> m_shptr;
		std::unique_lock<async_shared_timed_mutex_type> m_unique_lock;

		friend class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteAccessRequester<_Ty>;
	};

	template<typename _Ty>
	class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer {
	public:
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer(TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer&& src) = default;
		virtual ~TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer() {}

		operator bool() const {
			//if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer")); }
			return m_shptr.operator bool();
		}
		const TAsyncSharedObj<const _Ty>& operator*() const {
			if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer")); }
			const TAsyncSharedObj<const _Ty>* extra_const_ptr = reinterpret_cast<const TAsyncSharedObj<const _Ty>*>(std::addressof(*m_shptr));
			return (*extra_const_ptr);
		}
		const TAsyncSharedObj<const _Ty>* operator->() const {
			if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer")); }
			const TAsyncSharedObj<const _Ty>* extra_const_ptr = reinterpret_cast<const TAsyncSharedObj<const _Ty>*>(std::addressof(*m_shptr));
			return extra_const_ptr;
		}
	private:
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr) : m_shptr(shptr), m_shared_lock(shptr->m_mutex1) {}
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t) : m_shptr(shptr), m_shared_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_shared_lock.try_lock()) {
				shptr = nullptr;
			}
		}
		template<class _Rep, class _Period>
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t, const std::chrono::duration<_Rep, _Period>& _Rel_time) : m_shptr(shptr), m_shared_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_shared_lock.try_lock_for(_Rel_time)) {
				shptr = nullptr;
			}
		}
		template<class _Clock, class _Duration>
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t, const std::chrono::time_point<_Clock, _Duration>& _Abs_time) : m_shptr(shptr), m_shared_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_shared_lock.try_lock_until(_Abs_time)) {
				shptr = nullptr;
			}
		}
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer<_Ty>& operator=(const TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer<_Ty>& _Right_cref) = delete;
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer<_Ty>& operator=(TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer<_Ty>&& _Right) = delete;

		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer<_Ty>* operator&() { return this; }
		const TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer<_Ty>* operator&() const { return this; }
		bool is_valid() const {
			bool retval = m_shptr.operator bool();
			return retval;
		}

		std::shared_ptr<TAsyncSharedObj<_Ty>> m_shptr;
		std::shared_lock<async_shared_timed_mutex_type> m_shared_lock;

		friend class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteAccessRequester<_Ty>;
	};

	template<typename _Ty>
	class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteAccessRequester {
	public:
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteAccessRequester(const TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteAccessRequester& src_cref) = default;

		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer<_Ty> writelock_ptr() {
			return TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer<_Ty>(m_shptr);
		}
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer<_Ty> try_writelock_ptr() {
			return TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer<_Ty>(m_shptr, std::try_to_lock);
		}
		template<class _Rep, class _Period>
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer<_Ty> try_writelock_ptr_for(const std::chrono::duration<_Rep, _Period>& _Rel_time) {
			return TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer<_Ty>(m_shptr, std::try_to_lock, _Rel_time);
		}
		template<class _Clock, class _Duration>
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer<_Ty> try_writelock_ptr_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time) {
			return TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWritePointer<_Ty>(m_shptr, std::try_to_lock, _Abs_time);
		}
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer<_Ty> readlock_ptr() {
			return TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer<_Ty>(m_shptr);
		}
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer<_Ty> try_readlock_ptr() {
			return TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer<_Ty>(m_shptr, std::try_to_lock);
		}
		template<class _Rep, class _Period>
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer<_Ty> try_readlock_ptr_for(const std::chrono::duration<_Rep, _Period>& _Rel_time) {
			return TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer<_Ty>(m_shptr, std::try_to_lock, _Rel_time);
		}
		template<class _Clock, class _Duration>
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer<_Ty> try_readlock_ptr_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time) {
			return TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteConstPointer<_Ty>(m_shptr, std::try_to_lock, _Abs_time);
		}

		template <class... Args>
		static TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteAccessRequester make_asyncsharedobjectthatyouaresurehasnounprotectedmutablesreadwrite(Args&&... args) {
			//auto shptr = std::make_shared<TAsyncSharedObj<_Ty>>(std::forward<Args>(args)...);
			std::shared_ptr<TAsyncSharedObj<_Ty>> shptr(new TAsyncSharedObj<_Ty>(std::forward<Args>(args)...));
			TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteAccessRequester retval(shptr);
			return retval;
		}

	private:
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteAccessRequester(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr) : m_shptr(shptr) {}

		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteAccessRequester<_Ty>* operator&() { return this; }
		const TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteAccessRequester<_Ty>* operator&() const { return this; }

		std::shared_ptr<TAsyncSharedObj<_Ty>> m_shptr;
	};

	template <class X, class... Args>
	TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteAccessRequester<X> make_asyncsharedobjectthatyouaresurehasnounprotectedmutablesreadwrite(Args&&... args) {
		return TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteAccessRequester<X>::make_asyncsharedobjectthatyouaresurehasnounprotectedmutablesreadwrite(std::forward<Args>(args)...);
	}


	template<typename _Ty>
	class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer {
	public:
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer(TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer&& src) = default;
		virtual ~TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer() {}

		operator bool() const {
			//if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer")); }
			return m_shptr.operator bool();
		}
		const TAsyncSharedObj<const _Ty>& operator*() const {
			if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer")); }
			const TAsyncSharedObj<const _Ty>* extra_const_ptr = reinterpret_cast<const TAsyncSharedObj<const _Ty>*>(std::addressof(*m_shptr));
			return (*extra_const_ptr);
		}
		const TAsyncSharedObj<const _Ty>* operator->() const {
			if (!is_valid()) { throw(std::out_of_range("attempt to use invalid pointer - mse::TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer")); }
			const TAsyncSharedObj<const _Ty>* extra_const_ptr = reinterpret_cast<const TAsyncSharedObj<const _Ty>*>(std::addressof(*m_shptr));
			return extra_const_ptr;
		}
	private:
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer(std::shared_ptr<const TAsyncSharedObj<_Ty>> shptr) : m_shptr(shptr), m_shared_lock(shptr->m_mutex1) {}
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t) : m_shptr(shptr), m_shared_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_shared_lock.try_lock()) {
				shptr = nullptr;
			}
		}
		template<class _Rep, class _Period>
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t, const std::chrono::duration<_Rep, _Period>& _Rel_time) : m_shptr(shptr), m_shared_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_shared_lock.try_lock_for(_Rel_time)) {
				shptr = nullptr;
			}
		}
		template<class _Clock, class _Duration>
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer(std::shared_ptr<TAsyncSharedObj<_Ty>> shptr, std::try_to_lock_t, const std::chrono::time_point<_Clock, _Duration>& _Abs_time) : m_shptr(shptr), m_shared_lock(shptr->m_mutex1, std::defer_lock) {
			if (!m_shared_lock.try_lock_until(_Abs_time)) {
				shptr = nullptr;
			}
		}
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer<_Ty>& operator=(const TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer<_Ty>& _Right_cref) = delete;
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer<_Ty>& operator=(TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer<_Ty>&& _Right) = delete;

		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer<_Ty>* operator&() { return this; }
		const TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer<_Ty>* operator&() const { return this; }
		bool is_valid() const {
			bool retval = m_shptr.operator bool();
			return retval;
		}

		std::shared_ptr<const TAsyncSharedObj<_Ty>> m_shptr;
		std::shared_lock<async_shared_timed_mutex_type> m_shared_lock;

		friend class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyAccessRequester<_Ty>;
	};

	template<typename _Ty>
	class TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyAccessRequester {
	public:
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyAccessRequester(const TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyAccessRequester& src_cref) = default;
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyAccessRequester(const TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadWriteAccessRequester<_Ty>& src_cref) : m_shptr(src_cref.m_shptr) {}

		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer<_Ty> readlock_ptr() {
			return TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer<_Ty>(m_shptr);
		}
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer<_Ty> try_readlock_ptr() {
			return TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer<_Ty>(m_shptr, std::try_to_lock);
		}
		template<class _Rep, class _Period>
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer<_Ty> try_readlock_ptr_for(const std::chrono::duration<_Rep, _Period>& _Rel_time) {
			return TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer<_Ty>(m_shptr, std::try_to_lock, _Rel_time);
		}
		template<class _Clock, class _Duration>
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer<_Ty> try_readlock_ptr_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time) {
			return TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyConstPointer<_Ty>(m_shptr, std::try_to_lock, _Abs_time);
		}

		template <class... Args>
		static TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyAccessRequester make_asyncsharedobjectthatyouaresurehasnounprotectedmutablesreadonly(Args&&... args) {
			//auto shptr = std::make_shared<const TAsyncSharedObj<_Ty>>(std::forward<Args>(args)...);
			std::shared_ptr<const TAsyncSharedObj<_Ty>> shptr(new const TAsyncSharedObj<_Ty>(std::forward<Args>(args)...));
			TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyAccessRequester retval(shptr);
			return retval;
		}

	private:
		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyAccessRequester(std::shared_ptr<const TAsyncSharedObj<_Ty>> shptr) : m_shptr(shptr) {}

		TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyAccessRequester<_Ty>* operator&() { return this; }
		const TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyAccessRequester<_Ty>* operator&() const { return this; }

		std::shared_ptr<const TAsyncSharedObj<_Ty>> m_shptr;
	};

	template <class X, class... Args>
	TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyAccessRequester<X> make_asyncsharedobjectthatyouaresurehasnounprotectedmutablesreadonly(Args&&... args) {
		return TAsyncSharedObjectThatYouAreSureHasNoUnprotectedMutablesReadOnlyAccessRequester<X>::make_asyncsharedobjectthatyouaresurehasnounprotectedmutablesreadonly(std::forward<Args>(args)...);
	}


	/* For "read-only" situations when you need, or want, the shared object to be managed by std::shared_ptrs we provide a
	slightly safety enhanced std::shared_ptr wrapper. The wrapper enforces "const"ness and tries to ensure that it always
	points to a validly allocated object. Use mse::make_readonlystdshared<>() to construct an
	mse::TReadOnlyStdSharedFixedConstPointer. And again, beware of sharing objects with mutable members. */
	template<typename _Ty>
	class TReadOnlyStdSharedFixedConstPointer : public std::shared_ptr<const _Ty> {
	public:
		TReadOnlyStdSharedFixedConstPointer(const TReadOnlyStdSharedFixedConstPointer& src_cref) : std::shared_ptr<const _Ty>(src_cref) {}
		virtual ~TReadOnlyStdSharedFixedConstPointer() {}
		/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
		explicit operator const _Ty*() const { return std::shared_ptr<const _Ty>::operator _Ty*(); }

		template <class... Args>
		static TReadOnlyStdSharedFixedConstPointer make_readonlystdshared(Args&&... args) {
			TReadOnlyStdSharedFixedConstPointer retval(std::make_shared<const _Ty>(std::forward<Args>(args)...));
			return retval;
		}

	private:
		TReadOnlyStdSharedFixedConstPointer(std::shared_ptr<const _Ty> shptr) : std::shared_ptr<const _Ty>(shptr) {}
		TReadOnlyStdSharedFixedConstPointer<_Ty>& operator=(const TReadOnlyStdSharedFixedConstPointer<_Ty>& _Right_cref) = delete;

		//TReadOnlyStdSharedFixedConstPointer<_Ty>* operator&() { return this; }
		//const TReadOnlyStdSharedFixedConstPointer<_Ty>* operator&() const { return this; }
	};

	template <class X, class... Args>
	TReadOnlyStdSharedFixedConstPointer<X> make_readonlystdshared(Args&&... args) {
		return TReadOnlyStdSharedFixedConstPointer<X>::make_readonlystdshared(std::forward<Args>(args)...);
	}


	static void s_ashptr_test1() {
	}
}

#endif // MSEASYNCSHARED_H_

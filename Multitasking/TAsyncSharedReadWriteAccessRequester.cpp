
#include "mseasyncshared.h"
#include <ctime>
#include <ratio>
#include <chrono>
#include <future>
#include <iostream>

class H {
public:
	template<class _TAsyncSharedReadWriteAccessRequester>
	static double foo7(_TAsyncSharedReadWriteAccessRequester A_ashar) {
		auto t1 = std::chrono::high_resolution_clock::now();
		/* A_ashar.readlock_ptr() will block until it can obtain a read lock. */
		auto ptr1 = A_ashar.readlock_ptr(); // while ptr1 exists it holds a (read) lock on the shared object
		auto t2 = std::chrono::high_resolution_clock::now();
		std::this_thread::sleep_for(std::chrono::seconds(1));
		auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
		auto timespan_in_seconds = time_span.count();
		auto thread_id = std::this_thread::get_id();
		return timespan_in_seconds;
	}
protected:
	~H() {}
};

void main(int argc, char* argv[]) {
	/* The TAsyncShared data types are used to safely share objects between asynchronous threads. */

	class A {
	public:
		A(int x) : b(x) {}
		virtual ~A() {}

		int b = 3;
		mse::nii_string s = "some text ";
	};
	/* User-defined classes need to be declared as (safely) shareable in order to be accepted by the access requesters. */
	typedef mse::rsv::TAsyncShareableAndPassableObj<A> ShareableA;

	class B {
	public:
		static double foo1(mse::TAsyncSharedV2ReadWriteAccessRequester<ShareableA> A_ashar) {
			auto t1 = std::chrono::high_resolution_clock::now();
			/* mse::TAsyncSharedV2ReadWriteAccessRequester<ShareableA>::writelock_ptr() will block until it can obtain a write lock. */
			auto ptr1 = A_ashar.writelock_ptr(); // while ptr1 exists it holds a (write) lock on the shared object
			auto t2 = std::chrono::high_resolution_clock::now();
			std::this_thread::sleep_for(std::chrono::seconds(1));
			auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
			auto timespan_in_seconds = time_span.count();
			auto thread_id = std::this_thread::get_id();
			//std::cout << "thread_id: " << thread_id << ", time to acquire write pointer: " << timespan_in_seconds << " seconds.";
			//std::cout << std::endl;

			ptr1->s = std::to_string(timespan_in_seconds);
			return timespan_in_seconds;
		}
		static int foo2(mse::TAsyncSharedV2ImmutableFixedPointer<ShareableA> A_immptr) {
			return A_immptr->b;
		}
		static int foo3(mse::TAsyncSharedV2AtomicFixedPointer<int> int_atomic_ptr) {
			(*int_atomic_ptr) += 1;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			return *int_atomic_ptr;
		}
	protected:
		~B() {}
	};

	std::cout << std::endl;
	std::cout << "AsyncSharedV2 test output:";
	std::cout << std::endl;

	{
		/* This block contains a simple example demonstrating the use of mse::TAsyncSharedV2ReadWriteAccessRequester
		to safely share an object between threads. */

		std::cout << "TAsyncSharedV2ReadWriteAccessRequester:";
		std::cout << std::endl;
		auto ash_access_requester = mse::make_asyncsharedv2readwrite<ShareableA>(7);
		ash_access_requester.writelock_ptr()->b = 11;
		int res1 = ash_access_requester.readlock_ptr()->b;

		{
			auto ptr3 = ash_access_requester.readlock_ptr();
			auto ptr1 = ash_access_requester.writelock_ptr();
			auto ptr2 = ash_access_requester.writelock_ptr();
		}

		std::list<mse::mstd::future<double>> futures;
		for (size_t i = 0; i < 3; i += 1) {
			futures.emplace_back(mse::mstd::async(B::foo1, ash_access_requester));
		}
		int count = 1;
		for (auto it = futures.begin(); futures.end() != it; it++, count++) {
			std::cout << "thread: " << count << ", time to acquire read pointer: " << (*it).get() << " seconds.";
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
	{
		std::cout << "TAsyncSharedV2ReadOnlyAccessRequester:";
		std::cout << std::endl;
		auto ash_access_requester = mse::make_asyncsharedv2readonly<ShareableA>(7);
		int res1 = ash_access_requester.readlock_ptr()->b;

		std::list<mse::mstd::future<double>> futures;
		for (size_t i = 0; i < 3; i += 1) {
			futures.emplace_back(mse::mstd::async(J::foo7<mse::TAsyncSharedV2ReadOnlyAccessRequester<ShareableA>>, ash_access_requester));
		}
		int count = 1;
		for (auto it = futures.begin(); futures.end() != it; it++, count++) {
			std::cout << "thread: " << count << ", time to acquire read pointer: " << (*it).get() << " seconds.";
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
	{
		/* Just demonstrating the existence of the "try" versions. */
		auto access_requester = mse::make_asyncsharedv2readwrite<mse::nii_string>("some text");
		auto writelock_ptr1 = access_requester.try_writelock_ptr();
		if (writelock_ptr1) {
			// lock request succeeded
			int q = 5;
		}
		auto readlock_ptr2 = access_requester.try_readlock_ptr_for(std::chrono::seconds(1));
		auto writelock_ptr3 = access_requester.try_writelock_ptr_until(std::chrono::steady_clock::now() + std::chrono::seconds(1));
	}
	{
		/* TAsyncSharedV2WeakReadWriteAccessRequester<> is the weak counterpart to TAsyncSharedV2ReadWriteAccessRequester<>
		analogous to how std::weak_ptr<> is the weak counterpart to std::shared_ptr<>. */

		typedef decltype(mse::make_asyncsharedv2readwrite<mse::nii_string>("abc")) access_requester_t;
		auto vec1 = mse::mstd::vector<access_requester_t>();
		vec1.push_back(mse::make_asyncsharedv2readwrite<mse::nii_string>("abc"));

		mse::TAsyncSharedV2WeakReadWriteAccessRequester<mse::nii_string> weak_ar1(vec1.at(0));

		/* Here we're obtaining a (strong) access requester from the weak access requester, then appending it the
		vector of access requesters. */
		vec1.push_back(weak_ar1.try_strong_access_requester().value());

		assert((*(vec1.at(1).readlock_ptr())) == "abc");

		vec1.clear();

		/* All the (strong) access requesters have just been destroyed so attempting to obtain a (strong) access requester
		from our weak one will result in an empty optional being returned. */
		assert(!(weak_ar1.try_strong_access_requester().has_value()));
	}
	{
		/* For scenarios where the shared object is immutable (i.e. is never modified), you can get away without using locks
		or access requesters. */
		auto A_immptr = mse::make_asyncsharedv2immutable<ShareableA>(5);
		int res1 = A_immptr->b;
		std::shared_ptr<const ShareableA> A_shptr(A_immptr);

		std::list<mse::mstd::future<int>> futures;
		for (size_t i = 0; i < 3; i += 1) {
			futures.emplace_back(mse::mstd::async(B::foo2, A_immptr));
		}
		int count = 1;
		for (auto it = futures.begin(); futures.end() != it; it++, count++) {
			int res2 = (*it).get();
		}

		auto A_b_safe_cptr = mse::make_const_pointer_to_member(A_immptr->b, A_immptr);
	}
	{
		/* For scenarios where the shared object is atomic, you can get away without using locks
		or access requesters. */
		auto int_atomic_ptr = mse::make_asyncsharedv2atomic<int>(5);
		int res1 = (*int_atomic_ptr);

		std::list<mse::mstd::future<int>> futures;
		for (size_t i = 0; i < 3; i += 1) {
			futures.emplace_back(mse::mstd::async(B::foo3, int_atomic_ptr));
		}
		int count = 1;
		for (auto it = futures.begin(); futures.end() != it; it++, count++) {
			int res2 = (*it).get();
		}
	}
	{
		/* mse::TAsyncSharedV2ReadWriteAccessRequester's flexibilty in allowing coexisting read and write lock
		pointers in the same thread introduces new ways to produce logical deadlocks. This block (likely)
		demonstrates the access requester's ability to detect these potential deadlocks (and throw an exception
		when they would occur). */

		std::cout << "TAsyncSharedV2ReadWriteAccessRequester deadlock detection:";
		std::cout << std::endl;

		class CC {
		public:
			static void foo1(mse::TAsyncSharedV2ReadWriteAccessRequester<ShareableA> A_ashar, int id) {
				auto readlock_ptr = A_ashar.readlock_ptr();
				std::this_thread::sleep_for(std::chrono::seconds(1));
				try {
					auto writelock_ptr = A_ashar.writelock_ptr();
					std::this_thread::sleep_for(std::chrono::seconds(1));
				}
				catch (...) {
					// likely exception due to potential deadlock
					std::cout << "deadlock detected ";
					std::cout << std::endl;
				}
			}
		};

		auto ash_access_requester = mse::make_asyncsharedv2readwrite<ShareableA>(7);

		{
			auto thread1 = mse::mstd::thread(CC::foo1, ash_access_requester, 1);
			auto thread2 = mse::mstd::thread(CC::foo1, ash_access_requester, 2);
			thread1.join();
			thread2.join();
		}

		std::cout << std::endl;
	}
}

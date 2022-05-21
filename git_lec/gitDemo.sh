

#!/bin/bash

echo "Setup bare repo..."

mkdir gitDemoCentral

cd gitDemoCentral

git init --bare --initial-branch=main gitDemo.git

cd ..


echo "Create Bob's repo..."

git clone gitDemoCentral/gitDemo.git gitDemoBob

cd gitDemoBob

git config --local user.email bob@gmail.com

git config --local user.name bob

echo "Commit A" >> README.md && git add README.md && git commit -am "A"

git push

git switch -c branch_1

echo "Commit E" >> README.md && git commit -am "E"

echo "Commit F" >> README.md && git commit -am "F"

cd ..

echo "Create Anne's repo..."

git clone gitDemoCentral/gitDemo.git gitDemoAnne

cd gitDemoAnne

git config --local user.email anne@gmail.com

git config --local user.name anne

git remote add bob ../gitDemoBob

echo "Commit B" >> README.md && git commit -am "B"

git branch branch_2

echo "Commit C" >> README.md && git commit -am "C"

git switch branch_2

echo "Commit G" >> README.md && git commit -am "G"

echo "Commit H" >> README.md && git commit -am "H"

git switch main

#git log --oneline --graph --all

#gitk --all &


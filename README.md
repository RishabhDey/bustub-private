<img src="logo/bustub-whiteborder.svg" alt="BusTub Logo" height="200">

-----------------

[![Build Status](https://github.com/bsb20/bustub/actions/workflows/cmake.yml/badge.svg)](https://github.com/bsb20/bustub/actions/workflows/cmake.yml)

This is a fork of BusTub used for COMP 421 at UNC Chapel Hill.

BusTub is a relational database management system built at [Carnegie Mellon University](https://db.cs.cmu.edu) for the [Introduction to Database Systems](https://15445.courses.cs.cmu.edu) (15-445/645) course. This system was developed for educational purposes and should not be used in production environments.

BusTub supports basic SQL and comes with an interactive shell. You can get it running after finishing all the course projects.

<img src="logo/sql.png" alt="BusTub SQL" width="400">

**WARNING: IF YOU ARE A STUDENT IN THE CLASS, DO NOT DIRECTLY FORK THIS REPO. DO NOT PUSH PROJECT SOLUTIONS PUBLICLY. THIS IS AN HONOR CODE VIOLATION, EVEN AFTER THE COURSE HAS ENDED.**


**IF YOU ARE A NOT A STUDENT, PLEASE DO NOT MAKE YOUR SOLUTION PUBLICLY AVAILABLE.** Thank you for creating a fair learning environment.

## Cloning this Repository

The following instructions are adapted from the Github documentation on [duplicating a repository](https://docs.github.com/en/github/creating-cloning-and-archiving-repositories/creating-a-repository-on-github/duplicating-a-repository). The procedure below walks you through creating a private BusTub repository that you can use for development.

1. Go [here](https://github.com/new) to create a new repository under your account. Pick a name (e.g. `bustub-private`) and select **Private** for the repository visibility level.
2. On your development machine, create a bare clone of the public BusTub repository:
   ```console
   $ git clone --bare https://github.com/bsb20/bustub.git bustub-public
   ```
3. Next, [mirror](https://git-scm.com/docs/git-push#Documentation/git-push.txt---mirror) the public BusTub repository to your own private BusTub repository. Suppose your GitHub name is `STUDENT` and your repository name is `bustub-private`. The procedure for mirroring the repository is then:
   ```console
   $ cd bustub-public
   
   # If you pull / push over HTTPS
   $ git push https://github.com/STUDENT/bustub-private.git master

   # If you pull / push over SSH
   $ git push git@github.com:STUDENT/bustub-private.git master
   ```
   This copies everything in the public BusTub repository to your own private repository. You can now delete your local clone of the public repository:
   ```console
   $ cd ..
   $ rm -rf bustub-public
   ```
4. Clone your private repository to your development machine:
   ```console
   # If you pull / push over HTTPS
   $ git clone https://github.com/STUDENT/bustub-private.git

   # If you pull / push over SSH
   $ git clone git@github.com:STUDENT/bustub-private.git
   ```
5. Add the public BusTub repository as a second remote. This allows you to retrieve changes from this repository and merge them with your solution throughout the semester:
   ```console
   $ git remote add public https://github.com/bsb20/bustub.git
   ```
   You can verify that the remote was added with the following command:
   ```console
   $ git remote -v
   origin	https://github.com/STUDENT/bustub-private.git (fetch)
   origin	https://github.com/STUDENT/bustub-private.git (push)
   public	https://github.com/bsb20/bustub.git (fetch)
   public	https://github.com/bsb20/bustub.git (push)
   ```
6. You can now pull in changes from the public BusTub repository as needed with:
   ```console
   $ git pull public master
   ```
7. **Disable GitHub Actions** from the project settings of your private repository, otherwise you may run out of GitHub Actions quota.
   ```
   Settings > Actions > General > Actions permissions > Disable actions.
   ```

We suggest working on your projects in separate branches. If you do not understand how Git branches work, [learn how](https://git-scm.com/book/en/v2/Git-Branching-Basic-Branching-and-Merging). If you fail to do this, you might lose all your work at some point in the semester, and nobody will be able to help you.

## Build

We recommend developing BusTub on Ubuntu 22.04, or using a provided container image of Ubuntu 22.04. We do not officially support any other environments (i.e., do not open issues or come to office hours to debug them). We do not support WSL. You may be able to build the project on other Ubuntu versions, as well as MacOS, but do so at your own risk.  The grading environment runs
Ubuntu 22.04.

### Linux (Recommended) / macOS (Experimental)

To ensure that you have the proper packages on your machine, run the following script to automatically install them:

```console
# Linux
$ sudo build_support/packages.sh
# macOS
$ build_support/packages.sh
```

Then run the following commands to build the system:

```console
$ mkdir build
$ cd build
$ cmake ..
$ make
```

If you want to compile the system in debug mode, pass in the following flag to cmake:
Debug mode:

```console
$ cmake -DCMAKE_BUILD_TYPE=Debug ..
$ make -j`nproc`
```
This enables [AddressSanitizer](https://github.com/google/sanitizers) by default.

If you want to use other sanitizers,

```console
$ cmake -DCMAKE_BUILD_TYPE=Debug -DBUSTUB_SANITIZER=thread ..
$ make -j`nproc`
```

There are some differences between macOS and Linux (i.e., mutex behavior) that might cause test cases
to produce different results in different platforms. We recommend students to use a Linux VM for running
test cases and reproducing errors whenever possible.

While there are many ways to run in a Linux environment these days, we provide two ways for students to do this using Docker.
The first step for both options is to follow the instructions for [setting up Docker](https://docs.docker.com/get-started/get-docker/) on your host machine.

### Option 1: Docker
If you prefer to develop directly in a Linux environment, you can start and attach to a pre-configure container with all necessary packages installed by running:
```./docker_exec.sh```
This script will create and set up a container image for bustub, or attach to a running bustub container if one already exists.

Some MacOS users have reported a `command not found: docker` error when first running this script.  If you see this error, you most likely need to [add docker to your PATH](https://stackoverflow.com/questions/64009138/docker-command-not-found-when-running-on-mac).

### Option 2: VS Code Dev Containers
If you prefer to use VS Code, this repository is set up to integrate with the VS Code Dev Containers extension.
You can use this by:
1.  Installing the [Dev Containers](https://code.visualstudio.com/docs/devcontainers/containers) extension

2.  In VS Code, from the command palette (<kbd>F1</kbd>), select ***Dev Containers: Clone Repository in Named Container Volume***

3.  Enter a name for the named docker volume (can be any name, something like bustub-volume)

4.  Confirm the name of your working directory (should be auto-populated by VS Code)

**WARNING: If you are not prompted for a volume name as in step (3) above, you selected the wrong Dev Containers option in step 2.  Please select the named volume option to prevent data loss.**

You should now be able to develop, test, and commit code directly from the container or VS Code.

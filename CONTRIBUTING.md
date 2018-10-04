# Contributing Guide

## Getting Started

### Fork

All contributions to Open Sea should be made via GitHub pull requests, which require that the contributions be offered from a fork of the project's repository.

For help with forking a repository, refer to GitHub [Fork a Repo](https://help.github.com/articles/fork-a-repo/ "Fork a Repo").

### Build

Before you do anything else, please make sure that you have a functional build of the project to build upon. This can help prevent any future misunderstanding.

To make sure that the build you have is functional, run the attached tests. Instructions on how to run the tests should be part of the project's README file.

## Development

### Workflow

1. Create Feature Branch

```
$ git checkout -b feature/my-awesome-improvement
```

2. Make and Commit Changes

Edit the source and commit the changes you make in logical chunks (see Commit Guidelines).

3. Push to Fork

In order to submit a pull request, you must push your local branch into the forked repository.

```
$ git push origin feature/my-awesome-improvement
```

4. Submit a Pull Request

Submit a pull request from your feature branch onto the target branch of the project (See GitHub [Using Pull Requests](https://help.github.com/articles/about-pull-requests/ "Using Pull Requests")).

### Commit Guidelines

We encourage more small commits over one large commit. Small, focused commits make the review process easier and are more likely to be accepted. It is also important to summarise the changes made with brief commit messages. If the commit fixes a specific issue, it is also good to note that in the commit message.

The commit message should start with a single line that briefly describes the changes. That should be followed by a blank line and then a more detailed explanation. As a good practice, use commands when writing the message (instead of "I added ..." or "Adding ...", use "Add ...").

Before committing check for unnecessary whitespace with `git diff --check`.

For further recommendations, see [Pro Git Commit Guidelines](https://git-scm.com/book/en/v2/Distributed-Git-Contributing-to-a-Project#Commit-Guidelines "Pro Git Commit Guidelines").

### Code Standard
All code submitted should follow the project's [Code Standard](https://github.com/pilif0/open-sea/wiki/Code-Standard).
This is mainly to maintain a consistent style throughout the project and avoid potential bugs.

## Submitting Changes

### Pull Request Guidelines

The following guidelines can increase the likelihood that your pull request will get accepted:

* Work on topic branches.
* Follow the commit guidelines.
* Keep the patches on topic, focused, and atomic.
* Try to avoid unnecessary formatting and clean-up where reasonable.

A pull request should contain the following:

* At least one commit (all of which should follow the Commit Guidelines)
* Title that summarises the issue
* Description that briefly summarises the changes

After submitting a pull request, you should get a response within the next 7 days. If you do no, don't hesitate to ping the therad.

***
# Credits
This set of guidelines was inspired by the [Contributor Guidelines](https://apereo.github.io/cas/developer/Contributor-Guidelines.html) for CAS by Apereo.

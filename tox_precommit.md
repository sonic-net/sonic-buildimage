# Tox and pre-commit guide

## What is tox?

<span class="title-ref">Tox</span> is a tool written in Python to ease
tests automation and dependencies management. It provides a command line
tool that run tests inside a [Python virtual
environment](https://docs.python.org/3/glossary.html#term-virtual-environment).

This means that it will not modify your local system settings or package
and executable files. Instead, it uses a hidden folder
(<span class="title-ref">.tox</span>) to install the required Python
dependencies via pip ([the package installer for
Python](https://pip.pypa.io/)) before running tests.

You can find more details about tox at <https://github.com/tox-dev/tox>
. <span class="title-ref">Tox</span> is often used as a front-end to
Continuous Integration servers.

<span class="title-ref">Tox</span> configuration and behavior is very
portable across GNU+Linux distributions and others UNIX-like systems or
environments.

Once tox installed in a local environment with for example the following
command on Debian based systems:

    $ sudo apt-get install tox

or on Red Hat based systems:

    $ sudo yum install python-tox

the same test suite than the CI can be run locally by simply calling the
tox command from the project git clone local folder:

    $ tox

## Tox configuration

<span class="title-ref">Tox</span> configuration is written in the
<span class="title-ref">tox.ini</span> file at the root folder of the
Git project. Please read [tox official
documentation](https://tox.readthedocs.io/) for more details. For tox
users, the most important parameter in the
<span class="title-ref">\[tox\]</span> section is
<span class="title-ref">envlist</span>. It specifies which profiles to
run by default (i.e. when tox is called without the option
<span class="title-ref">-e</span>). The option
<span class="title-ref">-e</span> overrides this parameter and allows to
choose which profiles to run. For example:

    $ tox -e pre-commit-ci

will only run the <span class="title-ref">precommit-ci</span> profile. And:

    $ tox -e pre-commit-ci,pre-commit

will run the <span class="title-ref">pre-commit-ci</span> and
<span class="title-ref">pre-commit</span> profiles.

## Pre-commit profiles

[Pre-commit](https://pre-commit.com/) is a wrapper for various linters
that relies on [Git Hooks](https://git-scm.com/docs/githooks). It is
particularly useful to address common programming issues such as triming
trailing whitespaces or removing tabs.

<span class="title-ref">Pre-commit</span> configuration can be found in
the <span class="title-ref">.pre-commit-config.yaml</span> file at the
Git project root. <span class="title-ref">Pre-commit</span> hooks, like
any other Git hooks, are run automatically when the command
<span class="title-ref">'git commit'</span> is called. This avoids
forgetting running linters. Although,
<span class="title-ref">pre-commit</span> can also be called directly
from its shell command. This is what is currently performed in SONiC
Azure CI.

The <span class="title-ref">pre-commit</span> profile allows to call
<span class="title-ref">pre-commit</span> inside tox virtualenv without
installing the <span class="title-ref">pre-commit</span> package in the
local system, what is pretty convenient:

    $ tox -e pre-commit

This is also true to install/uninstall the corresponding Git hooks in
your Git folder thanks to the profiles
<span class="title-ref">pre-commit-install</span> and
\`pre-commit-uninstall\`:

    $ tox -e pre-commit-install

and:

    $ tox -e pre-commit-uninstall

To avoid linting all files in the repo in the CI, a dedicated profile
<span class="title-ref">pre-commit-ci</span> has been created to
lint only files modified or created after pre-commit introduction.

    $ tox -e pre-commit-ci

As specified in the envlist, this is the only profile run by default
when launching tox without any option.

Linters versions in the <span class="title-ref">.pre-commit-config.yaml</span>
file can be automatically updated to their latest version by running the
<span class="title-ref">pre-commit-autoupdate</span> profile:

    $ tox -e pre-commit-autoupdate

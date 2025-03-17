# ToDo list for gnucash-coowner extension

* WIP: Rebase and adpapt to v5.10
  * [X] enable ui pages: DistributionList
  * [X] enable ui pages: Co-Owner
  * [ ] import-export: coowner-import/dialog-coowner-import.c
  * [ ] toolbar menu: Co-Owner Overview
  * [ ] Dropdown menu: Co-Owner Overview

* WIP: cleanup failing tests
  * [X] test-business
  * [X] test-business-core
  * [ ] test-backend-dbi
    * not ok /backend/dbi/sqlite3/store_and_reload - gnc.io-FATAL-WARNING: [billterm_scrub()] deleting grandchild billterm: b92e0642f6934f1c8292ace3d66543ad
  * [ ] test-engine
    * ERROR:/data/development/gnucash/gnucash-develop/libgnucash/engine/test/utest-Account.cpp:2626:void test_xaccAccountType_Compatibility(): assertion failed (compat == currency_compat): (1055 == 224)
  * [X] test-distriblist
  * [X] test-query
  * [X] test-job
  * [X] test-aqb
  * [ ] test-invoice
    * 69: FAILURE number of built in invoice templates does not match /data/development/gnucash/gnucash-develop/gnucash/gnome/test/test-invoice-report-builtin-default.cpp:83 
    * 69: FAILURE built in default invoice guid not found /data/development/gnucash/gnucash-develop/gnucash/gnome/test/test-invoice-report-builtin-default.cpp:84 
    * 69: FAILURE built in Printable Invoice not found with default guid /data/development/gnucash/gnucash-develop/gnucash/gnome/test/test-invoice-report-builtin-default.cpp:85
    * 101: [fail] line:255, test: inv-8 combo amounts are correct
    * 101: test-invoice.scm/combinations of gncEntry options
    * 101:  -> expected: ("$2,133.25" "$2,061.96" "$2,133.25" "$2,061.96" "$2,133.25" "$2,133.25" "$1,851.95" "$1,859.30" "$16,368.17" "$1,111.01" "$17,479.18" "-$17,479.18" "$0.00")
    * 101:  -> obtained: ("$2,133.25" "$2,061.96" "$2,133.25" "$2,061.96" "$2,133.25" "$2,133.25" "$1,851.95" "$1,859.30" "$16,368.17" "$1,111.01" "$17,479.18" "$17,479.18")
* [ ] test-new-owner-report
* [ ] test-owner-report
* [ ] test-gnc-option-scheme-output
    * 128: test-gnc-option-scheme-output.scm:601:27: In procedure test-gnc-owner-option-to-scheme:
    * 128: Unbound variable: cdb. It is a typo, or inaccessible in current module.

* [ ] python-bindings

* distribution list: Assign a distribution key to a book
  * [ ]  define the distribution key the 100% value
  * [ ]  define the unit share for the property unit
  * [ ]  are we able to calculate the correct costs to be settled
  * [ ]  are we able to use different distribution keys per book or it's descendant book-childs
  * [ ]  assign a selection box for a distribution key in the dialog-coowner 'property info'

* co-owner: new frame 'tenant information'
  * [ ] attribut 'tenant since',
  * [ ] frame 'tenant address'

* co-owner: new attributes
  * [X] 'mobile phonenumber'
  * [ ] 'sip/instant messanger' url

# Building

## latest devolopment target

To facilidate the task, you may define a short bash-function. Here you can preset
the appropriate environment variables

* gnucash_root
  here you will `git clone` the stable branch from github origin.

* project_build
  Define a specific build directory for compiled libraries and binary objects.
  This keeps the source directory clean.

* project_build_type
  Define a specific build type.
  Choose `Debug` to keep `gdb symbols` needed to debug the code.
  Or choose `Release` for a production environment.

* project_build_generator
  that is a matter of tast. `man 7 cmake-generators` will offer supported values.
  Here: `ninja` or `make`

* project_install_prefix
  The final root directory, where the gnucash runtime files will be installed.
  If you do not want to infere with official packages, you might use to install
  into `/usr/local`.

```
gnucash_cmake_debug ()
{
    project_root=${1:-/data/development/gnucash/gnucash-develop}
    project_build=${2:-$project_root/../build-coowner-5}
    project_build_generator=${3:-Ninja}
	project_buid_type=${4:-Debug}
	project_install_prefix=${5-./install}
    project_cpp_flags=${6-"-O0"}

    if [ -d "$project_root" ]; then
        cd "project_root"
    else
        printf "Please download the guncash sources! Given project root-dir '%s' does not exist" "$project_root"
        exit 1
    fi

    if [ -d "$project_build" ]; then
        printf "Cleanup given project build-dir '%s'" "$project_build"
        cd $project_build
        rm -rf *
    else
        printf "Create new project build-dir: %s" "$project_build"
        mkdir "$project_build"
    fi

    cd "$project_build"
    cmake -D CMAKE_BUILD_TYPE="$project_build_type" \
    -D CMAKE_CPP_FLAGS="$project_install_prefix" \
    -D WITH_PYTHON=OFF \
    -D CMAKE_INSTALL_PREFIX="$project_install_prefix" \
    -G "$project_build_generator" \
    -S "$project_root"
}

gnucash_cmake_debug

ninja | make
```

To produce the production environment, you shoud adapt the

  * `project_build_type`=Release
  * `CMAKE_CPP_FLAGS`=-O2
  * `CMAKE_INSTALL_PREFIx`=/usr/local

## Arch Linux package

There are two `CoOwner` aware branches is hosted at GitHub.
Please have a look at

```
https://github.com/rzerres/gnucash.git
```

### Branch `coowner-5`

This branch will hold the latest version that is rebased on `gnucash v5.10` and compiles.
The CI Actions will give you some insights.

Clone the given `tag` with the following command:

```
git clone --branch 5.10-coowner https://github.com/rzerres/gnucash.git
```

### Branch `wip-coowner-5`

This branch is work in progress. It might brake at any stage.
I try to push only non-destructive states to GitHub.

Clone the latest commit via:

```
git clone --branch wip-coowner-5 https://github.com/rzerres/gnucash.git
```

## Debug Run

Referencing an Mailresponse from [John Ralls]( https://lists.gnucash.org/pipermail/gnucash-devel/2023-May/046711.html)
it is a bit tedious to test on a system with an installed version of Gnucash.
To overcome this problem, you can use the following to envieonment varialbles:

```
GNC_BUILDDIR=/tmp/gnucash \
GNC_UNINSTALLED=1 \
LANG=en_US.UTF8
LD_LIBRARY_PATH=/tmp/gnucash/lib/:/tmp/gnucash/lib/gnucash/ GUILE_LOAD_PATH=/tmp/gnucash/share/guile/site/3.0/ /tmp/gnucash/bin/gnucash
```

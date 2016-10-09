# Multiload-ng extras

This file describes contents of **extras** subdirectory in Multiload-ng source.

## Contents
- [checkinstall](#checkinstall) - Generate Debian, Slackware or RPM packages through `checkinstall`
- [pacman](#pacman) - Generate PKGBUILD for all variants of Multiload-ng
- [portage](#portage) - Ready to use Gentoo ebuilds

## checkinstall
Generate Debian, Slackware or RPM packages through `checkinstall`

Debian packages are used on Debian and its derivatives, Ubuntu and its
derivatives, Linux Mint, and so on.

RPM packages are used on Red Hat Linux, Fedora, Mandriva, SuSe, OpenSUSE,
PCLinuxOS, Mageia, and so on.

### Prerequisites
You will need `make` executable. Since you will have to build Multiload-ng
before creating package, you already have it.

You will also need **checkinstall** executable. It's likely that you will have to
install it. Refer to your distribution documentation to install **checkinstall**.

### Usage
First, configure and build Multiload-ng with desired options. Detailed
instructions are in main README.

Then, `cd` into *extras/checkinstall* directory, and execute:
- `make deb-package` to generate Debian package
- `make slack-package` to generate Slackware package
- `make rpm-package` to generate RPM package

Below there are the methods to install generated packages (you might need to run these as *root*):
- `dpkg -i <package.deb>` to install Debian package
- `installpkg <package.tgz>` to install Slackware package
- `rpm -i <package.rpm>` to install Slackware package


## pacman
Generate PKGBUILD for all variants of Multiload-ng

### Prerequisites
For creating PKGBUILDs, there is no prerequisite. Every Linux distro with a
Bash shell is just fine. `wget`, when present, allows to include MD5 sums
in generated PKGBUILDs, but it's optional.

Obviously, in order to use generated PKGBUILDs, you will need **Pacman** (default
package manager in Arch Linux and its derivatives)

### Usage
Simply `cd` into *extras/pacman* directory and execute included script:  
`./generate-pkgbuild-files.sh`

The script will run for a while, and then will generate several PKGBUILD files
with meaningful names, for each plugin/GTK+ version combination.

Script will generate PKGBUILDs for both last release and lastest git code.

For generating only one specific PKGBUILD, automatic generation of **.SRCINFO**,
and so on, run the script with `-h` option for details about available options:  
`./generate-pkgbuild-files.sh -h`

After generation, create packages for desired configurations with *makepkg*:  
`cd <desired_plugin_directory>; makepkg`

Install created packages with *pacman* (you might need to run as *root*):  
`pacman -U <package.pkg.tar.gz>`


## portage
Ready to use Gentoo ebuilds

### Prerequisites
You will need **Portage** (default package manager in Gentoo Linux and its
derivatives).

### Usage
First, `cd` into *extras/portage/x11-misc/multiload-ng* directory. Now, generate
the *Manifest*:  
`for i in *.ebuild; do ebuild $i manifest; done`

Now you can install Multiload-ng of desired version (you might need to run as *root*):  
`ebuild multiload-ng-<version>.ebuild merge`

There is also a *live ebuild*, which fetches always lastest git code.

These files are ready to be included as-is in your local overlay, if you have one!
Just copy whole *x11-misc* directory right under overlay root directory
(and maybe `chown portage`).
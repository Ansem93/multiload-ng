#!/usr/bin/env bash
#
# Copyright (C) 2016 Mario Cianciolo <mr.udda@gmail.com>
#
# This file is part of multiload-ng.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

pkgver=1.4.0
pkgrel=1

gen_md5sum()
{
	local tmpfile=$(mktemp)

	echo "Generating MD5 sums..." >&2

	if ! which wget > /dev/null
		then echo SKIP; return; fi

	if ! wget "https://github.com/udda/multiload-ng/archive/v$pkgver.tar.gz" -O "$tmpfile"
		then echo SKIP; return; fi

	md5sum $tmpfile | while read -a array; do echo ${array[0]} ; done

	rm "$tmpfile" >&2
}
md5sums=$(gen_md5sum)

get_pkgname()
{
	case "$1" in
		awn)		echo 'awn-applet-multiload-ng' ;;
		indicator)	echo 'multiload-ng-indicator' ;;
		lxpanel)	echo 'lxpanel-multiload-ng-applet' ;;
		mate)		echo 'mate-multiload-ng-applet' ;;
		standalone)	echo 'multiload-ng-standalone' ;;
		systray)	echo 'multiload-ng-systray' ;;
		xfce4)		echo 'xfce4-multiload-ng-plugin' ;;
	esac
}

get_pkgdesc()
{
	case "$1" in
		awn)		echo 'Modern graphical system monitor, Avant Window Navigator applet' ;;
		indicator)	echo 'Modern graphical system monitor, AppIndicator plugin' ;;
		lxpanel)	echo 'Modern graphical system monitor, LxPanel plugin' ;;
		mate)		echo 'Modern graphical system monitor, MATE panel applet' ;;
		standalone)	echo 'Modern graphical system monitor, standalone version' ;;
		systray)	echo 'Modern graphical system monitor, system tray version' ;;
		xfce4)		echo 'Modern graphical system monitor, XFCE4 panel plugin' ;;
	esac
}

get_depends()
{
	if [ "$2" = "gtk2" ]; then
		printf -- "'gtk2>=2.20.0' 'cairo'"
		case "$1" in
			awn)		printf -- " 'avant-window-navigator>=0.3.9' 'glibmm>=2.16.0' 'gtkmm>=2.20'" ;;
			indicator)	printf -- " 'libappindicator-gtk2>=0.4.92'" ;;
			lxpanel)	printf -- " 'lxpanel>=0.7.0' 'libfm-gtk2>=1.2.0'" ;;
			mate)		printf -- " 'mate-panel>=1.7.0'" ;;
			standalone)	return 0 ;;
			systray)	return 0 ;;
			xfce4)		printf -- " 'libxfce4util>=4.6.0' 'xfce4-panel>=4.6.0'" ;;
		esac
	else
		printf -- "'gtk3' 'cairo'"
		case "$1" in
			awn)		return 1 ;;
			indicator)	printf -- " 'libappindicator-gtk3>=0.4.92'" ;;
			lxpanel)	printf -- " 'lxpanel-gtk3>=0.7.0' 'libfm-gtk3>=1.2.0'" ;;
			mate)		printf -- " 'mate-panel-gtk3>=1.7.0'" ;;
			standalone)	return 0 ;;
			systray)	return 0 ;;
			xfce4)		printf -- " 'libxfce4util>=4.12.0' 'xfce4-panel>=4.12.0'" ;;
		esac
	fi
}

get_configure_string()
{
	printf -- '--prefix=/usr'

	if [ "$2" = "gtk2" ];
		then printf -- ' --with-gtk=2.0'
		else printf -- ' --with-gtk=3.0 --disable-deprecations'
	fi

	if [ "$1" = "awn" ]
		then printf -- ' --with-awn --enable-experimental'
		else printf -- ' --without-awn'
	fi
	if [ "$1" = "indicator" ]
		then printf -- ' --with-indicator'
		else printf -- ' --without-indicator'
	fi
	if [ "$1" = "lxpanel" ]
		then printf -- ' --with-lxpanel'
		else printf -- ' --without-lxpanel'
	fi
	if [ "$1" = "mate" ]
		then printf -- ' --with-mate'
		else printf -- ' --without-mate'
	fi
	if [ "$1" = "standalone" ]
		then printf -- ' --with-standalone'
		else printf -- ' --without-standalone'
	fi
	if [ "$1" = "systray" ]
		then printf -- ' --with-systray'
		else printf -- ' --without-systray'
	fi
	if [ "$1" = "xfce4" ]
		then printf -- ' --with-xfce4'
		else printf -- ' --without-xfce4'
	fi
}

generate_pkgbuild()
{
	local is_git=''
	local gtk_str='gtk2'
	local target=''

	for arg in "$@"; do
		case "$arg" in
			git)
				is_git=1 ;;
			gtk2 | gtk3)
				gtk_str="$arg" ;;
			awn | indicator | lxpanel | mate | standalone | systray | xfce4)
				target="$arg" ;;
		esac
	done


	# sanity check
	if [ -z "$target" ]; then
		echo "ERROR: no target specified" >&2
		return 1
	fi

	# there is no GTK3 support in AWN
	[ "$target" = "awn" -a "$gtk_str" = "gtk3" ] && return 2


	# additional parsing
	if [ -n "$is_git" ]; then
		local pkgver=`printf -- "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"`
		local version_str="git (${pkgver})"
		local git_suffix='-git'
		local makedepends="'intltool' 'git'"
		local pkg_source="git+https://github.com/udda/multiload-ng.git"
		local pkg_md5sum='SKIP'
		local pkg_basedir='multiload-ng'
	else
		local version_str="$pkgver"
		local git_suffix=''
		local makedepends="'intltool'"
		local pkg_source="https://github.com/udda/multiload-ng/archive/v\$pkgver.tar.gz"
		local pkg_md5sum="$md5sums"
		local pkg_basedir='multiload-ng-$pkgver'
	fi

	local _p="$(get_pkgname $target)"

	local pkgname="${_p}-${gtk_str}${git_suffix}"
	local pkgdesc="$(get_pkgdesc $target)"
	local depends="$(get_depends $target $gtk_str)"
	local configure_opts="$(get_configure_string $target $gtk_str)"

	# conflicts
	local conflicts=''
	for i in "${_p}-gtk2" "${_p}-gtk3" "${_p}-gtk2-git" "${_p}-gtk3-git"; do
		if [ ! "${pkgname}" = "$i" ]; then
			# appending with echo eliminates extra whitespaces
			conflicts=`echo ${conflicts} "'${i}'"`
		fi
	done

	# output
	printf -- "Generating PKGBUILD  (target: %-13s version: %-21s $gtk_str)" "$target" "$version_str" >&2
	local outdir="multiload-ng.PKGBUILD/${pkgname}"
	mkdir -p "${outdir}"

	cat >"${outdir}/PKGBUILD" <<-EOF
		# Maintainer: Mario Cianciolo <mr.udda at gmail dot com>
		# This file is part of multiload-ng.

		# THIS FILE IS GENERATED AUTOMATICALLY. DO NOT EDIT

		pkgname='${pkgname}'
		pkgdesc='${pkgdesc}'
		pkgver=${pkgver}
		pkgrel=${pkgrel}

		makedepends=(${makedepends})
		depends=(${depends})

		conflicts=(${conflicts})

		source=('${pkg_source}')
		md5sums=('${pkg_md5sum}')

		arch=('i686' 'x86_64')
		url='https://udda.github.io/multiload-ng/'
		license=('GPL2')

		build() {
		    cd "${pkg_basedir}"
		    ./autogen.sh
		    ./configure ${configure_opts}
		    make
		} 

		package() {
		    cd "${pkg_basedir}"
		    make -C "${target}" DESTDIR="\$pkgdir" install
		}
	EOF

	[ -n "$is_git" ] &&	cat >>"${outdir}/PKGBUILD" <<-EOF

		pkgver() {
		    cd "${pkg_basedir}"
		    printf -- "r%s.%s" "\$(git rev-list --count HEAD)" "\$(git rev-parse --short HEAD)"
		}
	EOF

	if which makepkg >/dev/null 2>&1
		then ( cd "${outdir}" ; makepkg --printsrcinfo > .SRCINFO )
	fi

	printf -- ' ... OK\n' >&2
}



for target in awn indicator lxpanel mate standalone systray xfce4; do
	for gtk in gtk2 gtk3; do
		generate_pkgbuild ${gtk} ${target}
		generate_pkgbuild ${gtk} ${target} git
	done
done

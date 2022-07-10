# Maintainer: Your Name <youremail@domain.com>
pkgname=tetris-git
pkgver=1.0.2
pkgrel=1
pkgdesc="A simple terminal Tetris game/linux demo project"
arch=(x86_64)
url="https://github.com/MMqd/tetris"
license=('GPL3')
makedepends=(git gcc make)
source=("https://github.com/MMqd/tetris/archive/refs/tags/"${pkgver}"v.tar.gz")
sha256sums=('SKIP')

package(){
	echo $PWD
	echo ${pkgdir}
	cd "tetris-${pkgver}v"
	make DESTDIR="${pkgdir}" install
}

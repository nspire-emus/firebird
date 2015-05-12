# Contributor: Antonio Vázquez Blanco <antoniovazquezblanco@gmail.com>

pkgname=nspire_emu
pkgver=0.9beta
pkgrel=1
pkgdesc='Community emulator of TI-Nspire calculators.'
arch=('i686' 'x86_64')
url='https://github.com/nspire-emus/nspire_emu'
license=()
depends=()
makedepends=()
source=('https://github.com/nspire-emus/nspire_emu/archive/v0.9-beta.tar.gz')
md5sums=('eed6d4e8151c8a2236619ebd9b1e8d2f')

build() {
  # Compile...
  cd "${srcdir}/${pkgname}-0.9-beta"
  mkdir -p build
  cd build
  qmake ..
  make
}

package() {
  # Creation of necesary folders...
  install -dm755 "${pkgdir}/usr/bin"

  # Copy the binary...
  install -D -m755 "${srcdir}/${pkgname}-0.9-beta/build/nspire_emu" "${pkgdir}/usr/bin/nspire_emu"
}


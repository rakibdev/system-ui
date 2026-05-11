pkgname=system-ui
pkgver=0.1.0
pkgrel=1
pkgdesc="UI for Linux"
arch=('x86_64')
license=('MPL-2.0')
depends=('gtk3' 'gtk-layer-shell' 'pipewire' 'cairo' 'libwebp' 'libjpeg-turbo' 'librsvg' 'glib2')
makedepends=('xmake' 'clang')

build() {
  cd "$startdir"
  xmake config --mode=release
  xmake build
}

package() {
  cd "$startdir"

  install -Dm755 build/ui               "$pkgdir/usr/bin/ui"
  install -Dm755 build/libsystem-ui.so  "$pkgdir/usr/lib/libsystem-ui.so"

  install -Dm644 src/default.css        "$pkgdir/usr/share/system-ui/default.css"

  install -Dm644 system-ui.service      "$pkgdir/usr/lib/systemd/user/system-ui.service"
}

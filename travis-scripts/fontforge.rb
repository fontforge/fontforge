# based on https://raw.githubusercontent.com/Homebrew/homebrew/master/Library/Formula/fontforge.rb

class MyDownloadStrategy < GitDownloadStrategy
  # get the PR
  def fetch
    system "echo", "Dumping to", cached_location
    system "rsync", "-a", "/Users/travis/build/fontforge/fontforge/.", cached_location
    # Mysterious fix for Homebrew/brew@2e916110e4c561b3e4175da099fc795e85ddb822
    version.update_commit(last_commit) if head?
  end

  def reset_args
    ref = case @ref_type
          when :branch then "origin/#@ref"
          when :revision, :tag then @ref
          else "origin/HEAD"
          end

    %W{reset --hard pr{TRAVIS_PULL_REQUEST}}
  end

  def reset
    quiet_safe_system "git", *reset_args
  end
end

class Fontforge < Formula
  desc "Command-line outline and bitmap font editor/converter"
  homepage "https://fontforge.github.io"
  url "https://github.com/fontforge/fontforge/archive/20160404.tar.gz"
  sha256 "1cc5646fccba2e5af8f1b6c1d0d6d7b6082d9546aefed2348d6c0ed948324796"
  head "file:///Users/travis/build/fontforge/fontforge", :branch => "FETCH_HEAD", :using => MyDownloadStrategy

  bottle do
    sha256 "74daacbb3416e84d8593fdaf2d0123ca4ef660bcbdcb5dda3792ac087cf07666" => :sierra
    sha256 "fd97cefd808fc0f07ac61e6ea624f074c9be5f2fb11f5a45468912fe5991ca36" => :el_capitan
    sha256 "e2dd2a2c7ce89b74b4bc2902da0ff93615b62b31bda5303a4f9bdf4447c2f05e" => :yosemite
    sha256 "6f1a9f1a0a15a2f84f0dce5c73e80e4265efd05e4bfa570f3c5e78da2211bbc6" => :mavericks
  end

  option "with-giflib", "Build with GIF support"
  option "with-extra-tools", "Build with additional font tools"

  deprecated_option "with-gif" => "with-giflib"

  # Autotools are required to build from source in all releases.
  depends_on "autoconf" => :build
  depends_on "automake" => :build
  depends_on "pkg-config" => :build
  depends_on "libtool" => :run
  depends_on "gettext"
  depends_on "pango"
  depends_on "zeromq"
  depends_on "czmq"
  depends_on "cairo"
  depends_on "fontconfig"
  depends_on "libpng" => :recommended
  depends_on "jpeg" => :recommended
  depends_on "libtiff" => :recommended
  depends_on "libspiro" => :recommended
  depends_on "libuninameslist" => :recommended
  depends_on "giflib" => :optional
  depends_on :python if MacOS.version <= :snow_leopard

  fails_with :llvm do
    build 2336
    cause "Compiling cvexportdlg.c fails with error: initializer element is not constant"
  end

  def install
    # Don't link libraries to libpython, but do link binaries that expect
    # to embed a python interpreter
    # https://github.com/fontforge/fontforge/issues/2353#issuecomment-121009759
    ENV["PYTHON_CFLAGS"] = `python-config --cflags`.chomp
    ENV["PYTHON_LIBS"] = "-undefined dynamic_lookup"
    python_libs = `python2.7-config --ldflags`.chomp
    inreplace "fontforgeexe/Makefile.am" do |s|
      oldflags = s.get_make_var "libfontforgeexe_la_LDFLAGS"
      s.change_make_var! "libfontforgeexe_la_LDFLAGS", "#{python_libs} #{oldflags}"
    end

    # Disable Homebrew detection
    # https://github.com/fontforge/fontforge/issues/2425
    inreplace "configure.ac", 'test "y$HOMEBREW_BREW_FILE" != "y"', "false"

    args = %W[
      --prefix=#{prefix}
      --enable-silent-rules
      --disable-dependency-tracking
      --without-x
    ]

    args << "--without-libpng" if build.without? "libpng"
    args << "--without-libjpeg" if build.without? "jpeg"
    args << "--without-libtiff" if build.without? "libtiff"
    args << "--without-giflib" if build.without? "giflib"
    args << "--without-libspiro" if build.without? "libspiro"
    args << "--without-libuninameslist" if build.without? "libuninameslist"
    #args << "--enable-gcc-warnings" if head?

    # Fix linker error; see: https://trac.macports.org/ticket/25012
    ENV.append "LDFLAGS", "-lintl"

    # Reset ARCHFLAGS to match how we build
    ENV["ARCHFLAGS"] = "-arch #{MacOS.preferred_arch}"

    # Bootstrap in every build: https://github.com/fontforge/fontforge/issues/1806
    system "./bootstrap"
    system "./configure", *args
    system "make"
    system "make", "install"

    if build.with? "extra-tools"
      cd "contrib/fonttools" do
        system "make"
        bin.install Dir["*"].select { |f| File.executable? f }
      end
    end
  end

  def post_install
    # Now we create a copy in /tmp that the script_osx.sh can use to
    # roll a package
    #
    # WARNING: using rsync runs into all sorts of troubles with autotools.
    #system "cp -aL . /tmp/fontforge-source-tree/"
  end

  test do
    system bin/"fontforge", "-version"
    system "python", "-c", "import fontforge"
  end
end

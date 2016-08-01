# based on https://raw.githubusercontent.com/Homebrew/homebrew/master/Library/Formula/fontforge.rb
#
# last synced by hand by DomT4 at 2015-06-18

class MyDownloadStrategy < GitDownloadStrategy
  # get the PR
  def fetch
    system "rsync -a /Users/travis/build/fontforge/fontforge/. ~/Library/Caches/Homebrew/fontforge--git"
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
  homepage "https://fontforge.github.io"
  url "https://github.com/fontforge/fontforge/archive/20150430.tar.gz"
  sha256 "430c6d02611c7ca948df743e9241994efe37eda25f81a94aeadd9b6dd286ff37"
  head "file:///Users/travis/build/fontforge/fontforge", :branch => "FETCH_HEAD", :using => MyDownloadStrategy
  revision 1

  bottle do
    revision 1
    sha256 "f8ad785c9a6e150d531571d572ccceb7e2a04073949c6c3508f91a9510b080f1" => :yosemite
    sha256 "7542f92ad89962181c0b1df6cfdf966d781274f86c797b940457c1b93d845b66" => :mavericks
    sha256 "bfc3c9062cbc8defca80a9660682f86df28541a349b064f7894648ae626ae1d5" => :mountain_lion
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
  depends_on "libpng" => :recommended
  depends_on "jpeg" => :recommended
  depends_on "libtiff" => :recommended
  depends_on "giflib" => :optional
  depends_on "libspiro" => :optional
  depends_on :python if MacOS.version <= :snow_leopard

  # This may be causing font-display glitches and needs further isolation & fixing.
  # https://github.com/fontforge/fontforge/issues/2083
  # https://github.com/Homebrew/homebrew/issues/37803
  depends_on "fontconfig"

  fails_with :llvm do
    build 2336
    cause "Compiling cvexportdlg.c fails with error: initializer element is not constant"
  end

  def install
    if MacOS.version <= :snow_leopard || !build.bottle?
      pydir = "#{%x(python-config --prefix).chomp}"
    else
      pydir = "#{%x(/usr/bin/python-config --prefix).chomp}"
    end

    # Please don't specify the insane verbosity of --disable-silent-rules
    args = %W[
      --prefix=#{prefix}
      --enable-silent-rules
      --disable-dependency-tracking
      --with-pythonbinary=#{pydir}/bin/python2.7
      --without-x
    ]

    args << "--without-libpng" if build.without? "libpng"
    args << "--without-libjpeg" if build.without? "jpeg"
    args << "--without-libtiff" if build.without? "libtiff"
    args << "--without-giflib" if build.without? "giflib"
    args << "--without-libspiro" if build.without? "libspiro"

    # Fix linker error; see: https://trac.macports.org/ticket/25012
    ENV.append "LDFLAGS", "-lintl"

    # Reset ARCHFLAGS to match how we build
    ENV["ARCHFLAGS"] = "-arch #{MacOS.preferred_arch}"

    # And for finding the correct Python, not always Homebrew's.
    ENV.prepend "CFLAGS", "-I#{pydir}/include"
    ENV.prepend "LDFLAGS", "-L#{pydir}/lib"
    ENV.prepend_path "PKG_CONFIG_PATH", "#{pydir}/lib/pkgconfig"

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
    system "cp -aL . /tmp/fontforge-source-tree/"
  end

  test do
    system bin/"fontforge", "-version"
  end
end

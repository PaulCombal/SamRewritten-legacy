SAM Rewritten for Linux
===

# Introduction

This is meant to be an equivalent to the popular program SAM Achievement Manager for Windows.
It's in a very early stage of development, and all contributions are welcome.

As of right now, all features are not implemented yet, but it can lock and unlock achievements.

Expect a lot of bugs and lacking features too.

Contributions are welcome!

Thanks :)

# Command line options

While SamRewritten offers a nice GUI, you linux geeks love to use command line options.
It would be exciting to expand the command line options, but I do not have much more time to invest
in this project, and a rewriting would not hurt. Anyway you can do:

* -a \<appid\>
	* `./bin/launch.sh -a 10` will idle Counter Strike

# Building

Just run ./make.sh. Makefiles? Never heard of it. Packaging? What's that?

Packages needed vary on distros, but make sure to install at least Steam, yajl, GTK 3, and GLib.

On Ubuntu (and variants): `sudo apt install libgtk-3-dev libcurl4-gnutls-dev libyajl-dev`

On Arch: `sudo pacman -S gtk3 glibc gnutls yajl`

To run it, launch ./bin/launch.sh

Once again, all contributions are VERY welcome, even though this code is already aging and very badly written.

# AUR Installation

You can install SamRewritten by using an AUR and installing the `samrewritten-git` package

Run SamRewritten by opening the corresponding desktop entry, or by using the `samrewritten` command.

### Footnotes

This software comes with no warranty. Use it at your own risk. We believe you should not get any ban using it, but we do not take any responsability over 
your user experience

<a name="readme-top"></a>
<div align="center">
	<a href="./LICENSE">
		<img alt="License" src="https://img.shields.io/badge/license-MIT-e8415e?style=for-the-badge">
	</a>
	<a href="https://github.com/LordOfTrident/pif/graphs/contributors">
		<img alt="Contributors" src="https://img.shields.io/github/contributors/LordOfTrident/pif?style=for-the-badge&color=f36a3b">
	</a>
	<a href="https://github.com/LordOfTrident/pif/stargazers">
		<img alt="Stars" src="https://img.shields.io/github/stars/LordOfTrident/pif?style=for-the-badge&color=efb300">
	</a>
	<a href="https://github.com/LordOfTrident/pif/issues">
		<img alt="Issues" src="https://img.shields.io/github/issues/LordOfTrident/pif?style=for-the-badge&color=0fae5e">
	</a>
	<a href="https://github.com/LordOfTrident/pif/pulls">
		<img alt="Pull requests" src="https://img.shields.io/github/issues-pr/LordOfTrident/pif?style=for-the-badge&color=4f79e4">
	</a>
	<br><br><br>
	<h1 align="center">PIF</h1>
	<p align="center">🎨 Palettized image format and software renderer 🖼️</p>
	<p align="center">
		<a href="#documentation">Documentation</a>
		.
		<a href="#demos">Demos</a>
		.
		<a href="todo.md">Todo</a>
		·
		<a href="https://github.com/LordOfTrident/pif/issues">Report Bug</a>
		·
		<a href="https://github.com/LordOfTrident/pif/issues">Request Feature</a>
	</p>
	<br>
</div>

<details>
	<summary>Table of contents</summary>
	<ul>
		<li><a href="#introduction">Introduction</a></li>
		<li>
			<a href="#demos">Demos</a>
			<ul>
				<li>
					<a href="#pre-requisites">Pre-requisites</a>
					<ul>
						<li><a href="#debian">Debian</a></li>
						<li><a href="#arch">Arch</a></li>
					</ul>
				</li>
				<li><a href="#debian">Quickstart</a></li>
			</ul>
		</li>
		<li><a href="#documentation">Documentation</a></li>
		<li><a href="#bugs">Bugs</a></li>
	</ul>
</details>

## Introduction
While making a not-yet-published retro 256-color 3D raycaster engine, i found myself writing a lot
of code for the palettized software rendering, image/palette file format loading and text rendering.
I needed to make utilities to convert from .bmp files to my palettized image files, utilities to
view these palettized image files and more.

I realised i am probably going to do more 256-color software rendering in the future, so i decided to
make a library for it to handle all of these things like image files, palettes, software rendering,
converting etc. Then i can make utilities for working with these image files, and reuse all of this
for every 256-color software rendered game i make in the future. And so the idea of PIF was born.

## Demos
> Showcase gifs and videos coming soon.

### Pre-requisites
The demo programs use C/C++ and the SDL2 library, so the following is required:
- A C/C++ compiler
- Makefile
- [SDL2](https://github.com/libsdl-org/SDL)

#### Debian
```
$ apt install gcc g++ make libsdl2-dev
```

#### Arch
```
$ pacman -S gcc make sdl2
```

### Quickstart
C demos are located in [demos/c/](demos/c/) and C++ demos are located in [demos/cpp/](demos/cpp/)

```sh
$ git clone https://github.com/LordOfTrident/pif
$ cd pif/demos/c/
$ make
$ ./dots3d
```

## Documentation
Coming soon.

## Bugs
If you find any bugs, please, [create an issue and report them](https://github.com/LordOfTrident/pif/issues).

<br>
<h1></h1>
<br>

<div align="center">
	<a href="https://en.wikipedia.org/wiki/C_(programming_language)">
		<img alt="C99" src="https://img.shields.io/badge/C99-0069a9?style=for-the-badge&logo=c&logoColor=white">
	</a>
	<a href="https://www.libsdl.org/">
		<img alt="SDL2" src="https://img.shields.io/badge/SDL2-1d4979?style=for-the-badge&logoColor=white">
	</a>
	<p align="center">Made with ❤️ love</p>
</div>

<p align="right">(<a href="#readme-top">Back to top</a>)</p>
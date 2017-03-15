# Nginx-IIS-Patch

Patch your Nginx to look like an IIS

## With IIS Start page
![IIS Start page](https://lightsing.github.io/Nginx-IIS-Patch/img/iis.png)
## With IIS Error page
![IIS Error page](https://lightsing.github.io/Nginx-IIS-Patch/img/iiserr.png)

# How to use this patch

## Option A:

- Dowload the release file
- `dpkg -i nginx-core_1.10.0-0ubuntu0.16.04.4_amd64.deb nginx-common_1.10.0-0ubuntu0.16.04.4_all.deb nginx_1.10.0-0ubuntu0.16.04.4_all.deb`

## Option B:

- `sudo apt update && sudo apt-get install dpkg-dev build-essential`
- `apt-get source nginx`
- `patch -p1 < nginx-iis-1.10.0-ubuntu.patch`
- `cd nginx-1.10.0 && dpkg-buildpackage -b`

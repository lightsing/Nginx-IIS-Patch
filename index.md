## Nginx IIS Patch Project

Patch your Nginx to look like an IIS Server.

# With IIS Start Page

![IIS Start Page](/Nginx-IIS-Patch/img/iis.png)

# With IIS Error Page

![IIS Error Page](/Nginx-IIS-Patch/img/iiserr.png)

### Nmap Scan Result

![Nginx Scan](/Nginx-IIS-Patch/img/Nginx.png)
![Nginx-IIS Scan](/Nginx-IIS-Patch/img/Nginx-IIS.png)

# How to use this patch

## Option A:

- Dowload the release file
- `dpkg -i nginx-core_1.10.0-0ubuntu0.16.04.4_amd64.deb nginx-common_1.10.0-0ubuntu0.16.04.4_all.deb nginx_1.10.0-0ubuntu0.16.04.4_all.deb`

## Option B:

- `sudo apt update && sudo apt-get install dpkg-dev build-essential`
- `apt-get source nginx`
- `patch -p1 < nginx-iis-1.10.0-ubuntu.patch`
- `cd nginx-1.10.0 && dpkg-buildpackage -b`

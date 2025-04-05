# [MREFD](https://github.com/n7tae/mrefd) ref-dash

This is the dashboard as seen on [M17-M17 Reflector](https://ref.m17.link) to be used with mrefd.

### Version 1.3.0 - mrefd v0.3.0 and introducing health checks!

Non-breaking change to config.inc.php - if you intend on using the health checks, either create a new config file from config.inc.php.dist or insert the line containing $CallHome['GUID'] to your existing configuration file. Follow the instructions below for setting up health checks!

### Clone dashboard to /var/www

```bash
sudo rm /var/www/html
sudo git clone https://github.com/m17-project/ref-dash /var/www/html     # or where ever your system www root is located
```

Please note that your www root directory might be some place else. There is one file that needs configuration. Edit the copied files, not the ones from the repository:

```bash
cd /var/www/html/include
sudo cp config.inc.php.dist config.inc.php
```

### Customizations
- Homepage link
  - edit config.ini.php to change the logo to link to your homepage. Defaults to the M17 website.
- Logo file
  - place your logo in /images (SVG preferred) and edit config.ini.php to change the logo image. Defaults to the M17 logo.

### Health Checks - DEPRECATED AS OF 31 JAN 2021
The new reflectors list [https://reflectors.m17.link](https://reflectors.m17.link) is being coded to provide active checks from the M17 Reflectors server.

When this feature is finished, a status update will show here as well as be announced on the M17 Twitter feed.

### Files to edit
- **include/config.inc.php** 
  - ContactEmail - set this to the sysop's email address
  - IPV4 - set this to the IPv4 address of the reflector
  - IPV6 - set this to the IPv6 address of the reflector, if not used, enter NONE
  - Homepage - set this to your homepage, defaults to m17project
  - Logo - set this to the filename of your logo, defaults to M17 logo
  - LocalModification - set this to your local modification version number if you modify the main code
  <!-- - CallHome GUID - Set this to your assigned Health Check GUID -->

### Caveat

If you notice that the formatting of the page does not look correct, please be sure to clear your browser's cache!

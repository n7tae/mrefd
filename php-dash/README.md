# [MREFD](https://github.com/n7tae/mrefd) php-dash

This dashboard is a php alternative to the GO based "gomrefdash" dashboard.

### Version 1.4.1

You will need to install webserver software (such as Apache, nginx, lighttpd, and others) on the system hosting your reflector to use this dash. In addition, you will 
need the php/php-fpm packages as well. You can use the following instructions to install lighttpd and php on a debian based operating system:

```bash
sudo apt update ; sudo apt install -y lighttpd php php-fpm
```

Once the necessary packages are installed, issue the following commands, taking note that you will need to modify the ln command to point to the location that you have
cloned mrefd. /opt/This program is used in the example, so be sure to update this to the correct location of your mrefd installation.

```bash
sudo rm -rf /var/www/html
sudo lighttpd-enable-mod fastcgi-php-fpm
ln -s /opt/mrefd/php-dash /var/www/html
sudo systemctl restart lighttpd
```

### Files to edit

There is one file that needs configuration. Edit the copied files, not the ones from the repository:

```bash
cd /var/www/html/include
sudo cp config.inc.php.dist config.inc.php
```
You can use nano, vi, or any editor of your choice to edit the newly created config.inc.php file.

- **config.inc.php** 
  - ContactEmail - set this to the reflector owers emailaddress
  - IPV4 - set this to the IPv4 address of the reflector
  - IPV6 - set this to the IPv6 address of the reflector, if not used, enter NONE
  - LocalModification - set this to your local modification version number if you modify the main code
  - ModuleNames has a description for each module. Feel free to change the default values. 

If you have configured more than A, B, C, and D modules, you can add more by following the same format. Please take note of the format of each line when adding additional 
modules.

### Important Note

This is a very basic configuration of a light weight http server. This will not provide you with https (encrypted/secured) connections. That is outside of the scope of this tutorial.
With popular free certificate authorities such as ZeroSSL and LetsEncrypt available to all, and tools such as acme.sh to automate certificate renewal, we suggest configuring https for 
your reflector dashboard.

If you wish to use something other than lighttpd, feel free. Any of them should work, and this dashboard has been tested with Apache, nginx, and lighttpd.
 
### Caveat

If you notice that the formatting of the page does not look correct, please be sure to clear your browser's cache! 

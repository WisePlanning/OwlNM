Flash the 2017-05-12-wise-lite.img file to the sd card

#copy all required files to the RPi
scp -r * wise@192.168.0.?:

#login ログイン
ssh wise@192.168.0.?:

user: wise
pass: wise

sudo -s

echo 'wise ALL=(ALL:ALL) NOPASSWD:ALL' >> /etc/sudoers

vi /boot/cmdline.txt
-dwc_otg.lpm_enable=0 console=serial0,115200 console=tty1 root=ROOTDEV rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait
+dwc_otg.lpm_enable=0 console=serial0,115200 console=tty3 root=ROOTDEV rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait loglevel=0 quiet splash logo.nologo vt.cur_default=1

cp /usr/share/zoneinfo/Japan /etc/localtime

# enter the floor dir for this Pi
cd Floor?-?

chown root:root Config/*
mv Config/hostname /etc/
mv Config/hosts /etc/
mv Config/interfaces /etc/network/
chmod 644 Config/autologin@.service
mv Config/autologin@.service /etc/systemd/system/autologin@.service
ln -fs /etc/systemd/system/autologin@.service /etc/systemd/system/getty.target.wants/getty@tty1.service
mv Config/config /etc/kbd/

# OwlNM Software
cd OwlController
make
sudo make install

systemctl disable dhcpcd
systemctl enable networking
systemctl disable hciuart
systemctl disable avahi-daemon

## Floor 1
# For Master マスター
echo 'sudo OwlNM -s &' >> /home/wise/.bashrc
echo 'sudo OwlNM -ma 127.0.0.1 -t 120' >> /home/wise/.bashrc
# For Client Pi　クライアント
echo 'OwlNM -cA' >> /home/wise/.bashrc


## Floor 2
apt-get remove omxplayer
rm -rf /usr/bin/omxplayer /usr/bin/omxplayer.bin /usr/lib/omxplayer
wget http://omxplayer.sconde.net/builds/omxplayer_0.3.7~git20170130~62fb580_armhf.deb
dpkg -i omxplayer_0.3.7~git20170130~62fb580_armhf.deb
wget -O /usr/local/bin/omxplayer-sync https://github.com/turingmachine/omxplayer-sync/raw/master/omxplayer-sync
chmod 0755 /usr/local/bin/omxplayer-sync

# start on Master マスター
echo 'omxplayer-sync -mu -b /home/wise/Movies/main.mp4' >> /home/wise/.bashrc
# start on Client　クライアント
echo 'omxplayer-sync -lu -b /home/wise/Movies/main.mp4' >> /home/wise/.bashrc


set the cards to RO
apt-get update
apt-get -y upgrade
apt-get remove --purge -y wolfram-engine triggerhappy cron anacron logrotate dphys-swapfile xserver-common lightdm fake-hwclock bluez
insserv -r x11-common
insserv -r bootlogs
insserv -r alsa-utils
insserv -r fake-hwclock
systemctl disable systemd-random-seed
systemctl disable systemd-readahead-collect
apt-get autoremove -y --purge
apt-get install -y busybox-syslogd
dpkg --purge rsyslog
rm -rf /var/run /var/spool /var/lock
ln -s /tmp /var/run
ln -s /tmp /var/spool
ln -s /tmp /var/lock
rm -rf /var/lib/dhcp/
ln -s /tmp /var/lib/dhcp

tmpfs           /tmp            tmpfs   nosuid,nodev         0       0
tmpfs           /var/log        tmpfs   nosuid,nodev         0       0
tmpfs           /var/tmp        tmpfs   nosuid,nodev         0       0

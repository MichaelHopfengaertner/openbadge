FROM ubuntu

MAINTAINER Oren Lederman <orenled@mit.edu>
# Based on https://github.com/FruityLoopers/fruityfactory

RUN apt-get update && apt-get install -y \
	build-essential \
  lib32z1 \
  lib32ncurses5 \
  unzip \
  binutils-avr \
  git \
  cmake \
  minicom screen \
  vim \
  wget
  #libbluetooth-dev \
  #bluez \
  #bluetooth \
  #fails? rfkill \

RUN mkdir -p /opt/downloads

WORKDIR /opt

# GCC ARM
RUN wget https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update/+download/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2 -O downloads/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2
RUN tar -C /usr/local -xjf downloads/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2

# nRF SDK
ENV NRF_SDK_PATH=/opt/nrf/sdk/nrf_sdk_8_1
RUN mkdir -p $NRF_SDK_PATH
RUN wget https://developer.nordicsemi.com/nRF51_SDK/nRF51_SDK_v8.x.x/nRF51_SDK_8.1.0_b6ed55f.zip -O downloads/nRF51_SDK_8.1.0_b6ed55f.zip
RUN unzip -q downloads/nRF51_SDK_8.1.0_b6ed55f.zip -d $NRF_SDK_PATH

COPY compose/Makefile.posix $NRF_SDK_PATH/components/toolchain/gcc/Makefile.posix

# Fix missing library error in Ubuntu 12.04
#RUN ln -s /lib/x86_64-linux-gnu/libudev.so.0 /lib/x86_64-linux-gnu/libudev.so.1

# SEGGER
copy compose/JLink_Linux_V616i_x86_64.deb /opt/downloads/JLink_Linux_V616i_x86_64.deb
RUN dpkg -i downloads/JLink_Linux_V616i_x86_64.deb

# Install Adafruit Bluefruit
RUN apt-get update && apt-get install -y \
    python \
    python-dev \
    python-setuptools \
    gcc \
    build-essential \
    libglib2.0-dev
    #bluez
#    --no-install-recommends && \
#    rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y \
     python-dbus

RUN git clone https://github.com/adafruit/Adafruit_Python_BluefruitLE
WORKDIR /opt/Adafruit_Python_BluefruitLE
RUN python setup.py install

# -- trying now - do the isntallation iwth bluez 5.33
# https://github.com/adafruit/Adafruit_Python_BluefruitLE
RUN apt-get update && apt-get install -y \
     libusb-dev libdbus-1-dev libglib2.0-dev libudev-dev libical-dev libreadline-dev
# need to copy  /usr/share/pkgconfig/udev.pc  from host to docker!!!
# start the dbus service
# run /usr/local/bin/bluetoothd --experimental &


# ---- This this (make sure bluez is installed). Maybe install the other bluetooth sutff?
# if I do "/etc/init.d/dbus start" and "/usr/sbin/bluetoothd --experimental &" I'm getting:
# AttributeError: 'NoneType' object has no attribute 'power_on'
# so it's not getting the adapter...
#-------------------------
# Other stuff I tried:
# if I start dbus (/etc/init.d/dbus start) I get a different error message

# the problem - if python-dbus gets installed i'm getting the filenotfound error
#
# trying in my computer:
# sudo apt-get install libdbus-1-dev libdbus-glib-1-dev libgirepository1.0-dev gobject-introspection libcairo2-dev python-cairo python-gi
# sudo apt-get install python-pip
# pip install dbus-python gi pygobject --not needed if installing all of the above?
# pip install git+https://github.com/adafruit/Adafruit_Python_BluefruitLE.git


# added python-dbus tothe stuff above. need to remove it? need a newer version?
# then i'm getting a different error - ImportError: No module named gi.repository
# apt-get update, and then apt-get install --reinstall python-gi
# then again dbus.exceptions.DBusException: org.freedesktop.DBus.Error.FileNotFound: Failed to connect to socket /var/run/dbus/system_bus_socket: No such file or directory


# trying to install python-dbus with pip. saying:
# No package 'dbus-1' found. trying apt-get install libdbus-1-dev

#older -
# needed - apt-get update and then install python-dbus
# but then - dbus.exceptions.DBusException: org.freedesktop.DBus.Error.FileNotFound: Failed to connect to socket /var/run/dbus/system_bus_socket: No such file or directory


# possible - apt-get -y install libusb-dev libdbus-1-dev libglib2.0-dev libudev-dev libical-dev libreadline-dev


# cleanup
WORKDIR /opt
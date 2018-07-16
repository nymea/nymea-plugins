Pianohat on the Raspberry Pi 3

# Make sounds work

In order to make the sounds work on the raspberry pi jack output you have to do following steps:

- Add the sound module to the default modules

        $ echo "snd-bcm2835" | sudo tee -a /etc/modules

- Run `raspi-config`
    - `7 Advanced Options`
    - `A4 Audio`
    - `1 Force 3,5mm (headphone) jack`

## Install pulseaudio

    $ apt update
    $ apt install pulseaudio

Create the systemd service:

    $ nano /lib/systemd/system/pulseaudio.service

Insert folowing lines

    [Unit]
    Description=PulseAudio Sound System
    Before=sound.target
    
    [Service]
    Type=simple
    ExecStart=/usr/bin/pulseaudio --daemonize=no --system --realtime --log-target=journal
    
    [Install]
    WantedBy=session.target


Configure pulseaudio for systemwide usage

    $ nano /etc/pulse/client.conf

Add following configurations

    ...
    default-server = /var/run/pulse/native
    autospawn = no

Add all users to the pulseaudio groups

    $ nano /etc/group

    ...
    pulse:x:114:nymea,root
    pulse-access:x:115:nymea,root

Finally reboot the system


# Test audio

Make sure your master PCM volume is up. You can do that using

    $ alsamixer

You shoudl see follwoing audio outputs:

    $ aplay -L
    
    default
        Playback/recording through the PulseAudio sound server
    null
        Discard all samples (playback) or generate zero samples (capture)
    sysdefault:CARD=ALSA
        bcm2835 ALSA, bcm2835 ALSA
        Default Audio Device
    dmix:CARD=ALSA,DEV=0
        bcm2835 ALSA, bcm2835 ALSA
        Direct sample mixing device
    dmix:CARD=ALSA,DEV=1
        bcm2835 ALSA, bcm2835 IEC958/HDMI
        Direct sample mixing device
    dsnoop:CARD=ALSA,DEV=0
        bcm2835 ALSA, bcm2835 ALSA
        Direct sample snooping device
    dsnoop:CARD=ALSA,DEV=1
        bcm2835 ALSA, bcm2835 IEC958/HDMI
        Direct sample snooping device
    hw:CARD=ALSA,DEV=0
        bcm2835 ALSA, bcm2835 ALSA
        Direct hardware device without any conversions
    hw:CARD=ALSA,DEV=1
        bcm2835 ALSA, bcm2835 IEC958/HDMI
        Direct hardware device without any conversions
    plughw:CARD=ALSA,DEV=0
        bcm2835 ALSA, bcm2835 ALSA
        Hardware device with all software conversions
    plughw:CARD=ALSA,DEV=1
        bcm2835 ALSA, bcm2835 IEC958/HDMI
        Hardware device with all software conversions

Finally test if the jack connected speaker works:

    $ aplay /usr/share/sounds/alsa/Front_Center.wav



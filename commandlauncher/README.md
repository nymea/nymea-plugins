# Application and script launcher

This plugin allows you to execute system commands and start bash scripts.

## Supported Things

### Application launcher

The application launcher allows you to call bash applications or commands with parameters from nymea.
Once, the application started, the `running` state will change to `true`, if the application
is finished, the `running` state will change to `false`.

**Example**

An example command could be [espeak](http://linux.die.net/man/1/espeak). (`apt-get install espeak`)

    espeak -v en "Chuck Norris is using nymea"

### Bash script launcher

The bashscript launcher allows you to start a bash script (with parameters)
from nymea. Once, the script is running, the `running` state will change to `true`, if the application
is finished, the `running` state will change to `false`.

**Example**

An example for a very useful script could be a backup scrip like following `backup.sh` script.


    #!/bin/sh
    # Directories to backup...
    backup_files="/home /etc /root /opt /var/www /var/lib/jenkins"

    # Destination of the backup...
    dest="/mnt/backup"

    # Create archive filename...
    day=$(date +%Y%m%d)
    hostname="nymea.io"
    archive_file="$day-$hostname.tgz"

    # Print start status message...
    echo "Backing up $backup_files to $dest/$archive_file"
    date
    echo

    # Backup the files using tar.
    tar czf $dest/$archive_file $backup_files

    echo
    echo "Backup finished"
    date
    echo "==========================="
    echo "  DONE, have a nice day!   "
    echo "==========================="


To make the script executable use following command:

    chmod +x backup.sh

## Requirements

* The package “nymea-plugin-commandlauncher” must be installed.

## More

https://ubuntu.com/tutorials/command-line-for-beginners

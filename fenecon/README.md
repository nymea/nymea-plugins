# Fenecon
## FEMS
--------------------------------

This plugin reads data from the FEMS system.
You will receive information about the *status* of the FEMS system,
your production/consumption as well as status of your *battery*, if you have one.
Additionally your *meter* data is being fetched. Primarly showing you if you are feeding in the system,
or receive Power from the grid.
Please ensure that the REST api (Read only is enough) on your FEMS system is enabled.
This should be a default setting.
If not, please ask your installer to enable the REST Api.
If the connection should not work, update your FEMS REST Api configuration,
and set the default port to 8084.

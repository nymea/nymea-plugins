# Autocreated plot script for system disk usage
set term png small size 2000,1000
set output "multisensor.png"
set title "Multisensor filter"
set xlabel "Time"
set ylabel "Value"
set xdata time
set timefmt "%s"
set xtics format "%H:%M:%S"
set yrange [0:*]
set grid

plot "/tmp/multisensor.log" using 1:2 with lines title "Original value", \
     "/tmp/multisensor.log" using 1:3 with lines title "Filtered value"


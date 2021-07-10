set termoption enhanced
set grid
set style line 1 lc rgb "blue" lt 1 pt 1
set style line 2 lc rgb "red" lt 1 pt 2
set style line 3 lc rgb "green" lt 1 pt 3
set style line 4 lc rgb "black" lt 1 pt 6

set title 'Refinaments Vs. Total Mean Error'
set xlabel 'Size' font "arial,15"
set ylabel 'Total Mean Error' font "arial,15"
set terminal pdf font "arial,15"
set output 'refs_error_log.pdf'
set logscale x
set logscale y

plot 'refs.txt' u 1:2 ls 1 t 'Solver 8', 'refs.txt' u 1:3 ls 2 t 'Solver 9', 'refs.txt' u 1:4 ls 3 t 'Solver 11', 'refs.txt' u 1:5 ls 4 t 'Solver 12'

unset title
unset ylabel

set title 'Refinaments Vs. Execution Time'
set ylabel 'Time [min]' font "arial,15"
set output 'refs_time_log.pdf'

plot 'refs.txt' u 1:6 ls 1 t 'Solver 8', 'refs.txt' u 1:7 ls 2 t 'Solver 9', 'refs.txt' u 1:8 ls 3 t 'Solver 11', 'refs.txt' u 1:9 ls 4 t 'Solver 12'

unset title
unset ylabel

set title 'Refinaments Vs. Iterations'
set ylabel 'Iterations' font "arial,15"
set output 'refs_iter_log.pdf'

plot 'refs.txt' u 1:10 ls 1 t 'Solver 8', 'refs.txt' u 1:11 ls 2 t 'Solver 9', 'refs.txt' u 1:12 ls 3 t 'Solver 11', 'refs.txt' u 1:13 ls 4 t 'Solver 12'

unset title
unset ylabel
unset xlabel

set title 'Order Vs. Total Mean Error'
set xlabel 'Size' font "arial,15"
set ylabel 'Total Mean Error' font "arial,15"
set output 'order_error_log.pdf'

plot 'order.txt' u 1:2 ls 1 t 'Solver 8', 'order.txt' u 1:3 ls 2 t 'Solver 9', 'order.txt' u 1:4 ls 3 t 'Solver 11', 'order.txt' u 1:5 ls 4 t 'Solver 12'

unset title
unset ylabel

set title 'Order Vs. Execution Time'
set ylabel 'Time [min]' font "arial,15"
set output 'order_time_log.pdf'

plot 'order.txt' u 1:6 ls 1 t 'Solver 8', 'order.txt' u 1:7 ls 2 t 'Solver 9', 'order.txt' u 1:8 ls 3 t 'Solver 11', 'order.txt' u 1:9 ls 4 t 'Solver 12'

unset title
unset ylabel

set title 'Order Vs. Iterations'
set ylabel 'Iterations' font "arial,15"
set output 'order_iter_log.pdf'

plot 'order.txt' u 1:10 ls 1 t 'Solver 8', 'order.txt' u 1:11 ls 2 t 'Solver 9', 'order.txt' u 1:12 ls 3 t 'Solver 11', 'order.txt' u 1:13 ls 4 t 'Solver 12'

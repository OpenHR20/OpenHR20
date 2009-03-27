<?php

$trace_layout_ids_double = array (
    array( 'temp_average' , '' ),
    array( 'bat_average' , '' ),
    array( 'sumError' , '' ),
    array( 'CTL_temp_wanted' , '' ),
    array( 'CTL_temp_wanted_last' , '' ),
    array( 'CTL_temp_auto' , '' ),
    array( 'CTL_mode_auto' , '' ),
    array( 'motor_diag' , '' ),
    array( 'MOTOR_PosMax' , '' ),
    array( 'MOTOR_PosAct' , '' ),
    0xff => array( 'LAYOUT_VERSION' , '' )
);

foreach ($trace_layout_ids_double as $k=>$v) {
  $trace_layout_ids[$k]=$v[0];
  $trace_layout_names[$v[0]]=$k;
}

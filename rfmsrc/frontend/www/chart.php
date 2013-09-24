<?php

$addr=(int) $_GET['addr'];
$real=(int) $_GET['real'];
$wanted=(int) $_GET['wanted'];
$valve=(int) $_GET['valve'];
$hours=(int) $_GET['hours'];
if ($hours<=0) $hours=24;
$now = time();
$min_time = $now-$hours*60*60;

include 'config.php';
include 'lib/xy_chart.php';

$g = new XY_chart(600,300);

/*
for($i = 0; $i < 200; $i++ )
{
  $g->add(0,($i-100) * ($i-100),($i+50) * ($i-50) * ($i-10));
}
*/

$g->line_prop(0,'left' ,255,0,0);
$g->line_prop(1,'left' ,0,255,0);
$g->line_prop(2,'right',0,0,255);
$g->x_title='seconds in past';
$g->yl_title='T [C]';
$g->yr_title='V [%]';

$result = $db->query("SELECT * FROM log WHERE addr=$addr AND time>$min_time ORDER BY time DESC");

while ($row = $result->fetchArray()) {
    if ($real) $g->add(0,$row['time']-$now,$row['real']/100);
    if ($wanted) $g->add(1,$row['time']-$now,$row['wanted']/100);
    if ($valve) $g->add(2,$row['time']-$now,$row['valve']);
}

$g->plot();
$g->PNG();
unset($g);

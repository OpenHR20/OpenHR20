<?php 
  $actions = array (
    array( 0 => true, 'page'=>'status', 'text' => 'status'),
    array( 0 => false, 'page'=>'history', 'text' => 'history'),
    array( 0 => false, 'page'=>'timers', 'text' => 'timers'),
    array( 0 => false, 'page'=>'eeprom', 'text' => 'setting'),
    array( 0 => false, 'page'=>'trace', 'text' => 'trace points'),
    array( 0 => true, 'page'=>'queue', 'text' => 'queue'),
    array( 0 => true, 'page'=>'debug_log', 'text' => 'Debug LOG'),
    array( 0 => false, 'page'=>'raw_command_queue', 'text' => 'command queue - RAW'),
  );

global $RRD_ENABLE;
if ($RRD_ENABLE)
	$actions[] = array( 0 => true, 'page'=>'rrd', 'text' => 'RRD info');
?>



<h2>Action</h2>
<ul>
<?php
  foreach ($actions as $a) {
      if ($addr==0 && !$a[0]) continue;
      if ($page==$a['page']) 
	echo '<li class="active">';
      else 
	echo '<li>';
      echo '<a href="?page='.$a['page']."&amp;addr=$addr\">".$a['text'].'</a></li>';
  }
?>
</ul>

<h2>Valve</h2>
<ul>
<?php
  if ($addr==0) 
    echo '<li class="active">';
  else 
    echo '<li>';
  echo "<a href=\"?page=status&amp;addr=0\"><strong>all</strong></a></li>";
  foreach ($room_name as $a => $n) {
      if ($addr==$a) 
	echo '<li class="active">';
      else 
	echo '<li>';
      echo "<a href=\"?page=$page&amp;addr=$a\">$n</a></li>";
  }
?>
</ul>

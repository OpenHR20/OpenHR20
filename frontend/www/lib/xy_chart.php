<?php
// extreme lightweight XY graph library
// suficient for slow devices like linux based routers

class XY_chart {
    public $char_width = 8;
    public $char_height = 11;
    public  $x_title = 'x axis';
    public  $yl_title = 'y-l axis';
    public  $yr_title = 'y-r axis';

    private $x_size;
    private $y_size;
    private $x_start;
    private $y_start;
    private $x_end;
    private $y_end;
    private $right;
    private $bottom;
    private $left = 0;
    private $top = 0;
    private $min_x = 9e99;
    private $l_min_y = 9e99;
    private $r_min_y = 9e99;
    private $max_x = -9e99;
    private $l_max_y = -9e99;
    private $r_max_y = -9e99;
    private $gx=array();
    private $gy=array();
    private $line_color=array();
    private $y_axis=array(); // false=left, true=right

    public function __construct($x_size, $y_size) {
	$this->x_size = $x_size;
	$this->y_size = $y_size;
	$this->x_start = $this->x_left + 50;
	$this->y_start = $this->top + $this->char_height * 1.5;
	$this->x_end = $this->x_start + $this->x_size-50;
	$this->y_end = $this->y_start + $this->y_size;
	$this->right = $this->x_start + $this->x_size;
	$this->bottom = $this->y_start + $this->y_size + $this->char_height * 1.5;
    }
    
    public function line_prop($line,$y_axis,$r,$g,$b) {
      $this->line_color[$line]=array($r,$g,$b);
      $this->y_axis[$line] = ($y_axis=='right');
    }
    
    public function add ($line,$x,$y) {
      $this->gx[$line][]=$x;
      $this->gy[$line][]=$y;
      if ($x<$this->min_x) $this->min_x=trim($x)-1;
      if ($x>$this->max_x) $this->max_x=trim($x)+1;
      if ($this->y_axis[$line]) {
	if ($y<$this->r_min_y) $this->r_min_y=trim($y)-1;
	if ($y>$this->r_max_y) $this->r_max_y=trim($y)+1;
      } else {
	if ($y<$this->l_min_y) $this->l_min_y=trim($y)-1;
	if ($y>$this->l_max_y) $this->l_max_y=trim($y)+1;
      }
    }
    
    public function plot() {
      $max_x += $this->max_x * 0.05;
      $max_y += $this->max_y * 0.05;
      $this->image = ImageCreate($this->right - $this->left, $this->bottom - $this->top);
      $background_color = imagecolorallocate($this->image, 255, 255, 255);
      $text_color = imagecolorallocate($this->image, 233, 14, 91);
      $grey = ImageColorAllocate($this->image, 204, 204, 204);
      $white = imagecolorallocate($this->image, 255, 255, 255);
      $black = imagecolorallocate($this->image, 0, 0, 0);
      imagerectangle($this->image, $this->left, $this->top, $this->right - 1, $this->bottom - 1, $black );
      imagerectangle($this->image, $this->x_start, $this->y_start, $this->x_end, $this->y_end, $grey );
      
      foreach ($this->gx as $k=>$v) {
	if ($this->y_axis[$k]) {
	  $prev_pt_x = $this->x_start + ($this->x_end-$this->x_start)*($this->gx[$k][0]-$this->min_x)/($this->max_x-$this->min_x);
	  $prev_pt_y = $this->y_end - ($this->y_end - $this->y_start)*($this->gy[$k][0]-$this->r_min_y)/($this->r_max_y-$this->r_min_y);
	  $color = imagecolorallocate($this->image, $this->line_color[$k][0], $this->line_color[$k][1], $this->line_color[$k][2]);
	  foreach($this->gx[$k] as $i=>$v)
	  {
	    $pt_x = $this->x_start + ($this->x_end-$this->x_start)*($this->gx[$k][$i]-$this->min_x)/($this->max_x-$this->min_x);
	    $pt_y = $this->y_end - ($this->y_end - $this->y_start)*($this->gy[$k][$i]-$this->r_min_y)/($this->r_max_y-$this->r_min_y);

	    imageline( $this->image, $pt_x, $pt_y, $prev_pt_x, $prev_pt_y, $color );
	    $prev_pt_x = $pt_x;
	    $prev_pt_y = $pt_y;
	  }
	 } else {
	  $prev_pt_x = $this->x_start + ($this->x_end-$this->x_start)*($this->gx[$k][0]-$this->min_x)/($this->max_x-$this->min_x);
	  $prev_pt_y = $this->y_end - ($this->y_end - $this->y_start)*($this->gy[$k][0]-$this->l_min_y)/($this->l_max_y-$this->l_min_y);
	  $color = imagecolorallocate($this->image, $this->line_color[$k][0], $this->line_color[$k][1], $this->line_color[$k][2]);
	  foreach($this->gx[$k] as $i=>$v)
	  {
	    $pt_x = $this->x_start + ($this->x_end-$this->x_start)*($this->gx[$k][$i]-$this->min_x)/($this->max_x-$this->min_x);
	    $pt_y = $this->y_end - ($this->y_end - $this->y_start)*($this->gy[$k][$i]-$this->l_min_y)/($this->l_max_y-$this->l_min_y);

	    imageline( $this->image, $pt_x, $pt_y, $prev_pt_x, $prev_pt_y, $color );
	    $prev_pt_x = $pt_x;
	    $prev_pt_y = $pt_y;
	  }
	 }
      }


      $string = sprintf("%d", $this->l_max_y);
      imagestring($this->image, 4, $this->x_start - strlen($string) * $this->char_width, $this->y_start - $this->char_width, $string, $black);

      $string = sprintf("%d", $this->l_min_y);
      imagestring($this->image, 4, $this->x_start - strlen($string) * $this->char_width, $this->y_end - $this->char_height, $string, $black);

      $string = sprintf("%d", $this->r_max_y);
      imagestring($this->image, 4, $this->x_end + 4, $this->y_start - $this->char_width, $string, $black);

      $string = sprintf("%d", $this->r_min_y);
      imagestring($this->image, 4, $this->x_end + 4, $this->y_end - $this->char_height, $string, $black);

      $string = sprintf("%d", $this->min_x);
      imagestring($this->image, 4, $this->x_start - (strlen($string) * $this->char_width)/2, $this->y_end, $string, $black);
	
      $string = sprintf("%d", $this->max_x);
      imagestring($this->image, 4, $this->x_end - (strlen($string) * $this->char_width) / 2, $this->y_end, $string, $black);

      imagestring($this->image, 4, $x_start + ($this->x_end - $this->x_start) / 2 - strlen($this->x_title) * $this->char_width / 2, $this->y_end, $this->x_title, $black);

      imagestring($this->image, 4, $this->char_width, ($this->y_end - $this->y_start) / 2, $this->yl_title, $black);
      imagestring($this->image, 4, $this->x_end + $this->char_width, ($this->y_end - $this->y_start) / 2, $this->yr_title, $black);
    }

    public function PNG() {
      header('Content-type: image/png');
      ImagePNG($this->image);
    }
    
    public function __desctruct() {
      ImageDestroy($this->image);
    }

}

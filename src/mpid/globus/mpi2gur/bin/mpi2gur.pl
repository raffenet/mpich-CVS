#!/usr/bin/perl -w

# 
# Copyright 2007 Northern Illinois University
# 
# XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
# 

use Time::Local;

# ** KEY=VALUE **

my $inSubjob; 
my $inParm;
my $lvlBlocks;	# level of blocks we're in
my $curLine;
my $parseLine;
my $hasLine;
my $key;	# current key from RSL
my $value;	# current value from RSL

my @input;	# current line of input from RSL

my @allKeys;	# all keys from RSL
my @allValues;	# all values from RSL

# arrays containing values for machines_dict_string
my @gurMachines;
my @gurMachineMinNodes;
my @gurMachineMaxNodes;
my @gurMachineAccount;
my @gurMachineEmail;
my @gurMachineUser;

# time to add onto reservation (more than job max time)
my $RES_TIME_FUDGE = 300;

# individual GUR values
my $gurTotalNodes;
my $gurCurMachine;
my $gurDuration;
my $gurStart;
my $gurEnd;
my $gurUser = "";
my $gurAccount = "";
my $gurReorder = "no";
my $gurEmail = "\'NONOTIFY\'";

# Reservation time (in secs) if no maxtime specified
my $gurDurationDefault = 600;	# 10 minutes 
my $gurIndex;
my $gurMaxTime;

my $curMachine;
my $curResID;
my $curNumNodes;

# time values for stand and end dates
my $now;
my $nowPlusTwoWeeks;
my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst);

my $thisStartSecs;
my $latestStartSecs = 0;
my $timeDiff;

my $thisProject;
my $thisCount;
my $thisHostCount;

# file names
my $inRslFilename;	# RSL input	
my $outGurFilename;	# GUR output
my $outRslFilename;	# new RSL output

my ($i,$j);		# misc index

# months hash for coversions
my %months = ();
$months{"jan"} = 0;
$months{"feb"} = 1;
$months{"mar"} = 2;
$months{"apr"} = 3;
$months{"may"} = 4;
$months{"jun"} = 5;
$months{"jul"} = 6;
$months{"aug"} = 7;
$months{"sep"} = 8;
$months{"oct"} = 9;
$months{"nov"} = 10;
$months{"dec"} = 11;
my $monWord;

#########################################################################################
#
# runStage = 0 --> just create GUR jobfile (don't run GUR)
# runStage = 1 --> create GUR jobfile and run GUR to get reservation ID
# runStage = 2 --> same as 1, plus creates the new RSL file including the reservation ID
# runStage = 3 --> Create new RSL file and submit it (depending on wait) (default)
#
#########################################################################################

my $runStage = 3;

# Sleep until reservation starts (1)?  or just exit (0)?  
my $waitForRes = 1;
my $TIME_FUDGE = 15;  # 15 seconds

parseArguments();

# output GUR filename = input RSL filename + .gur
$outGurFilename = sprintf("%s.gur", $inRslFilename);

# open files
open (INPUT,$inRslFilename) || die "Cannot open $inRslFilename";

# Not starting within a subjob or parm
$inSubjob = 0;
$inParm = 0;
$lvlBlocks = 0;

$gurTotalNodes = 0;
$gurDuration = $gurDurationDefault;
$gurMaxTime = 0;

print "\nParsing $inRslFilename ...";

# parse RSL file, picking up EVERY key=value pair
while(<INPUT>)
{
  $curLine = $_;
  $hasLine = 1;

  while($hasLine)
  {
    if($inSubjob)
    {
      if($inParm)
      {
        $key = parseKey($curLine);
        $value = parseValue($curLine);

        chomp $key;
        chomp $value;

        @allKeys = (@allKeys, $key);
        @allValues = (@allValues, $value);

        if(parseBlockEnd($curLine) eq 0)
        { die "Expected )\n"; }
        
        $inParm = 0;
      }
      else
      {
        if(parseBlockBegin($curLine))
        { $inParm = 1; }
        else
        {
          if(parseBlockEnd($curLine))
          {
            $inSubjob = 0;

            # Extra space for reservation_id and delimiter
            @allKeys = (@allKeys, "##", "@@");
            @allValues = (@allValues, "##", "@@");
          }
          else
          { $hasLine = 0; }
        }
      }
    }
    else
    {
      if(parseBlockBegin($curLine))
      { $inSubjob = 1; }
      else
      {
        if(parseBlockEnd($curLine))
        { die "Unexpected )\n"; }

        # done with this line
        $hasLine = 0;
      }
    }
  }
}  

# done reading in RSL info
close INPUT;
print "done!\n";

# start converting RSL info into GUR stuff
$gurIndex = 0;

print "Converting RSL to GUR ...";

for($i = 0; $i < @allKeys; $i++)
{
  if($allKeys[$i] =~ /^maxtime$/i)
  {
    # pick the largest time maxtime of all subjobs
    my $thisMaxTime = $allValues[$i];
    $thisMaxTime =~ s/\"//g;

    if($gurMaxTime < $thisMaxTime)
    {
      $gurMaxTime = $thisMaxTime;

      # convert minutes to seconds
      $gurDuration = $gurMaxTime * 60 + $RES_TIME_FUDGE;
    }
  }
  elsif($allKeys[$i] =~ /^resourceManagerContact$/i)
  {
    # convert resource manager to GUR format
    $gurCurMachine = $allValues[$i];
    $gurCurMachine =~ s/\//#slash#/g;
    $gurCurMachine =~ s/\"//g;
  }
  elsif($allKeys[$i] =~ /^count$/i)
  {
    $thisCount = $allValues[$i];
    $thisCount =~ s/\"//g;

    $gurTotalNodes += $thisCount;
  }
  elsif($allKeys[$i] =~ /^hostcount$/i || $allKeys[$i] =~ /^host_count$/i)
  {
    $thisHostCount = $allValues[$i];
    $thisHostCount =~ s/\"//g;

    # if hostcode specifies node type (ie ia64-compute)
    #  then add node type to current GUR machine name
    if($thisHostCount =~ /:/)
    { $gurCurMachine = $gurCurMachine . "#" . $'; }
  }
  elsif($allKeys[$i] =~ /^project$/i)
  {
    $thisProject = $allValues[$i];
    $thisProject =~ s/\"//g;
  }
  elsif($allKeys[$i] =~ /^\@\@$/)
  # we're done with this machine; move on to next
  { 
    my $machineNdx = machineExists($gurCurMachine);

    if($machineNdx == -1)
    { 
      $gurMachines[$gurIndex] = $gurCurMachine; 

      # for now, min_nodes = max_nodes
      $gurMachineMinNodes[$gurIndex] = $thisCount;
      $gurMachineMaxNodes[$gurIndex] = $thisCount;
      
      $gurMachineProject[$gurIndex] = $thisProject;

      $gurIndex += 1; 
    }
    else
    {
      $gurMachineMinNodes[$machineNdx] += $thisCount;
      $gurMachineMaxNodes[$machineNdx] += $thisCount;
    }
  }
}

print "done!\n";

# done converting RSL info to GUR
# start writting GUR stuff out

print "Writing GUR jobfile $outGurFilename ...";
open (OUTGUR,sprintf(">%s", $outGurFilename)) || die "Cannot open $outGurFilename";
print OUTGUR "[metajob]\n";

# total_nodes
print OUTGUR "total_nodes = $gurTotalNodes\n";

# machine_preference
print OUTGUR "machine_preference = ";
for($i = 0; $i < @gurMachines; $i++)
{
  print OUTGUR "$gurMachines[$i]";

  # only print a comma if not last machine
  if($i < @gurMachines - 1)
  { print OUTGUR ","; }
}
print OUTGUR "\n";

# machine_preference_reorder
print OUTGUR "machine_preference_reorder = $gurReorder\n";

# duration
print OUTGUR "duration = $gurDuration\n";

# earliest_start and lastest_end
$now = time();
$nowPlusTwoWeeks = $now + (60 * 60 * 24 * 14);

($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($now);
$gurStart = sprintf("%02d:%02d_%02d/%02d/%04d", $hour, $min, $mon+1, $mday, $year+1900);

($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($nowPlusTwoWeeks);
$gurEnd = sprintf("%02d:%02d_%02d/%02d/%04d", $hour, $min, $mon+1, $mday, $year+1900);

print OUTGUR "earliest_start = $gurStart\n";
print OUTGUR "latest_end = $gurEnd\n";

# usage_pattern
print OUTGUR "usage_pattern = ";
if(@gurMachines > 1)
{ print OUTGUR "multiple\n"; }
else
{ print OUTGUR "single\n"; }

# machine_dict_string
print OUTGUR "machines_dict_string = {\n";
for($i = 0; $i < @gurMachines; $i++)
{
  print OUTGUR "\  '$gurMachines[$i]\' : {\n";
  
  # make sure array contains something if not defined
  if(!$gurMachineUser[$i])
  { $gurMachineUser[$i] = ""; }

  # remove '\n'
  chomp($gurMachineUser[$i]);

  # remove any '
  $gurMachineUser[$i] =~ s/\'//g;

  print OUTGUR sprintf("    \'username_string\' : \'%s\',\n", $gurMachineUser[$i]);

  # make sure array contains something if not defined
  if(!$gurMachineProject[$i])
  { $gurMachineProject[$i] = ""; }

  # remove any '
  $gurMachineProject[$i] =~ s/\'//g;

  print OUTGUR sprintf("    \'account_string\' : \'%s\',\n", $gurMachineProject[$i]);

  # email_notify
  print OUTGUR sprintf("    \'email_notify\' : %s,\n", $gurEmail);

  print OUTGUR sprintf("    \'min_int\' : %d,\n", $gurMachineMinNodes[$i]);
  print OUTGUR sprintf("    \'max_int\' : %d\n", $gurMachineMaxNodes[$i]);

  print OUTGUR "   }";

  # only print comma if not last machine
  if($i != @gurMachines - 1)
  { print OUTGUR ","; }
  print OUTGUR "\n";
}

print OUTGUR "  }\n";

# done printing out GUR stuff
close OUTGUR;
print "done!\n";

# **exit here if runStage is 0**
exit 0 if $runStage == 0;

# run GUR with new GUR jobfile
$gurCommand = "./reserve_it $outGurFilename";
print "running GUR <$gurCommand> ...";
@gurOutput = `$gurCommand`;

if ($? != 0)
{
  print "There was a problem running GUR!\n";
  print "@gurOutput\n";
  die "return code was $?\n";
}

print "done!\n";

# obtain GUR reservation id from GUR output
for($i = 0; $i < @gurOutput; $i++)
{
  #print "$i : $gurOutput[$i]\n";
  # if this line contains the reservation ID
  if ($gurOutput[$i] =~ /.*RESERVATION ID.*/)
  { 
    $curMachine = getGURMachineName($gurOutput[$i]);
    $curResID = getGURReservationID($gurOutput[$i]);

    $j = $i;
    while($j++ < @gurOutput)
    {
      if ($gurOutput[$j] =~ /.*node count.*/i)
      {
        $curNumNodes = getGURNumNodes($gurOutput[$j]);
        print "\nReservation: $curMachine with $curNumNodes nodes\n";
        print "  Reservation ID $curResID\n";
        print "  Start time $hour:$min:$sec $monWord $mday, $year\n";
        pushReservationID($curMachine, $curNumNodes, $curResID);

        last;
      }
      elsif ($gurOutput[$j] =~ /.*reservation start time.*/i)
      {
        # parse out start time components
        $_ = $gurOutput[$j];

        # seconds
        /:\d\d\s/;
        $sec = $&;
        $sec =~ s/://g;
        $sec =~ s/\s//g;

        # minute
        /:\d\d:/;
        $min = $&;
        $min =~ s/://g;

        # hour
        /\s\d\d:/;
        $hour = $&;
        $hour =~ s/://g;
        $hour =~ s/\s//g;

        # month day
        /\s[\d\s]\d\s/;
        $mday = $&;
        $mday =~ s/\s//g;

        # month
        /\s\w\w\w\s/;
        $monWord = $&;
        $monWord =~ s/\s//g;
        $mon = $months{lc($monWord)};

        # year
        /\s\d\d\d\d\)/;
        $year = $&;
        $year =~ s/\s//g;
        $year =~ s/\)//g;

        # convert to seconds
        $thisStartSecs = timelocal($sec, $min, $hour, $mday, $mon, $year);
        if($thisStartSecs > $latestStartSecs)
        { $latestStartSecs = $thisStartSecs; }
      }
    }
    $i = $j;
  }
  elsif($gurOutput[$i] =~ /bad account format/i)
  {
    print "GUR returned bad account format.  Please make sure account/project is valid in RSL\n";
    exit 4; 
  }
}

# **exit here if runStage is 1**
exit 0 if $runStage == 1;

# write new RSL file
# output RSL filename = _ + input RSL filename
$outRslFilename = sprintf("_%s", $inRslFilename);

# open output rsl
print "Writing new RSL file $outRslFilename ...";
open (OUTRSL,">$outRslFilename") || die "Cannot open $outRslFilename";

#if(@gurMachines > 1)
#{ print OUTRSL "+\n"; }
print OUTRSL "+\n"; 

for($i = 0; $i < @allKeys; $i++)
{
  print OUTRSL "\( &";
  while(1)
  {
    print OUTRSL "($allKeys[$i]=$allValues[$i])\n";
    $i++;
    if($allKeys[$i] =~ /^\@\@$/i)
    { last; }
  }
  print OUTRSL "\)\n";
}

close OUTRSL;
print "done!\n";

# **exit here if runStage is 2**
exit 0 if $runStage == 2;

# sleep until reservation starts (minus 10, just in case...)
#  only sleep if flag is set
$timeDiff = $latestStartSecs - time - 10;
if($timeDiff > 0 && $waitForRes == 1)
{ 
  print "\n  Reservation begins in $timeDiff seconds: $latestStartSecs - time() - 10\n";
  sleep($timeDiff); 
}

# submit new RSL to globus if it starts now
if($waitForRes == 0 && $timeDiff > $TIME_FUDGE)
{
  print " This reservation begins in $timeDiff seconds.  Run globusrun -f $outRslFilename then\n";
  exit 0;
}

print "\nglobusrun -f $outRslFilename\n";
system("globusrun -f $outRslFilename");

# delete new RSL file?? (reservation_id won't be valid after run...)
print "\nrm -f $outRslFilename\n";
#system("rm -f $outRslFilename");

## END OF PROGRAM ##


sub parseBlockBegin
{
  my $inLine;
  my $ndx;

  $inLine = $_[0];

  $ndx = index($inLine, "(");	# is there a ( ?
  if( $ndx ge 0 )
  {
    $_[0] = substr($inLine, $ndx + 1, length($inLine) - $ndx);
    return 1;
  }
  else
  {
    return 0;
  }
}

sub parseBlockEnd
{
  my $inLine;
  my $ndx;

  $inLine = $_[0];

  $ndx = index($inLine, ")");   # is there a ( ?
  if( $ndx ge 0 )
  {
    $_[0] = substr($inLine, $ndx + 1, length($inLine) - $ndx);
    return 1;
  }
  else
  {
    return 0;
  }
}

sub parseKey
{
  my $inLine;

  $inLine = $_[0];

  #remove leading spaces
  $inLine =~ s/^ *//;

  #remove everything after first non-word
  $inLine =~ s/\W.*//;

  return $inLine;
}

sub parseValue
{
  my $inLine;
  my $Ndx;
  my $length;
  my $openParenCount;
  my $isDone;
  my $value;

  $inLine = $_[0];

  # Get index for _first_ equals sign
  $Ndx = index($inLine, "=");

  # Discard everything before this equal sign
  $inLine = substr($inLine, $Ndx + 1, length($inLine) - $Ndx);

  #remove leading space
  $inLine =~ s/^ *//;
  
  $openParenCount = 1;
  $isDone = 0;

  while(1)
  {
    for($Ndx = 0; $Ndx < length($inLine); $Ndx++)
    {
      if(substr($inLine, $Ndx, 1) eq "\(")
      {
        $openParenCount += 1;
      } # if
      elsif(substr($inLine, $Ndx, 1) eq "\)")
      {
        $openParenCount -= 1;

        if($openParenCount == 0)
        {
          $value = $value . substr($inLine, 0, $Ndx);
          $isDone = 1;
          $_[0] = substr($inLine, $Ndx, length($inLine) - 1);
          last;
        } # if
      } # elsif 
    } # for
  
    if($isDone)
    {
      return $value;
    } # if($isDone)
  
    chomp($inLine);
    $value = $value . substr($inLine, 0, length($inLine));
    $inLine = <INPUT>;
  }  # while(1)
}

sub machineExists
{
  my $machineName = $_[0];
  my $ndx;

  for($ndx = 0; $ndx < @gurMachines; $ndx++)
  {
    if($gurMachines[$ndx] eq $machineName)
    {
      return $ndx;
    }
  }
 
  return -1;
}

sub getGURReservationID
{
  my $inLine = $_[0];
  my $id;
  my $Ndx = rindex($inLine, "\"");

  if($Ndx > 0)
  {
    $id = substr($inLine, $Ndx + 1);

    #remove leading spaces
    $id =~ s/^ *//;

    chomp($id);

    return $id;
  }
  else
  { return 0; }

}

sub getGURMachineName
{
  $_ = $_[0];
  /\".*\"/;

  if($&)
  {
    my $machine = $&;

    $machine =~ s/\"//g;
    return $machine;
  }
  else
  { return 0; }
}

sub getGURNumNodes
{
  $_ = $_[0];

  /node count \(.*\)/;

  if($&)
  {
    my $nodes = $&;

    $nodes =~ s/.*\(//g;
    $nodes =~ s/\)//g;
    return $nodes; 
  }
  else
  {
    return 0;
  }
}


sub pushReservationID
{
  ## IGNORES node count for now - only one reservation per machine ##

  my $inMachineName = $_[0];
  $_ = $inMachineName;
  /#slash#.*#/;
  my $nodeType = $';
  $nodeType =~ s/ //g;
  chomp($nodeType);
  $inMachineName =~ s/#slash#/\//g;
  $inMachineName =~ s/#.*//;

  my $inNumNodes = $_[1];
  my $inReservationID = $_[2];

  my $machineFound = 0;
  my $compareMachine;
  my $compareNodes;
  my $compareNodeType;

  for($i = 0; $i < @allKeys; $i++)
  {
    if($allKeys[$i] =~ /^resourceManagerContact$/i)    
    { 
      $compareMachine = $allValues[$i]; 
      $compareMachine =~ s/\"//g;
    }
    elsif($allKeys[$i] =~ /^count$/i)
    { 
      $compareNodes = $allValues[$i]; 
      $compareNodes =~ s/\"//g;
    }
    elsif($allKeys[$i] =~ /^hostcount$/i || $allKeys[$i] =~ /^host_count$/i)
    {
      $_ = $allValues[$i];
      /:/;
      $compareNodeType = $';
      $compareNodeType =~ s/ //g;
      chomp($compareNodeType);
    }
    elsif($allKeys[$i] =~ /^##$/)
    {
      ##if ($inMachineName =~ /$compareMachine/ && $inNumNodes =~ /$compareNodes/)
      if ($inMachineName =~ /$compareMachine/ && $compareNodeType eq $nodeType)
      {
        $allKeys[$i] = "reservation_id";
        $allValues[$i] = $inReservationID;
        $machineFound = 1;
        ##last;
      }
    }
    elsif($allKeys[$i] =~ /^\@\@$/)
    {
      $compareMachine = 0;
      $compareNodes = 0;
    }
  }

  if(!$machineFound)
  { print " Machine $inMachineName not found!\n"; }
}


sub parseArguments
{
  for($i = 0; $i <= $#ARGV; $i++)
  {
     $_ = $ARGV[$i];
     if(/^--/)
     {
       $_ = $';
       if(/^nowait$/i)
       { $waitForRes = 0; }
       elsif(/^wait$/i)
       { $waitForRes = 1; }
       elsif(/^create_gur$/i)
       { $runStage = 0; }
       elsif(/^run_gur$/i)
       { $runStage = 1; }
       elsif(/^create_rsl$/i)
       { $runStage = 2; }
       elsif(/^run_rsl$/i)
       { $runStage = 3; }
       else
       { die "Unknown flag $ARGV[$i]\n"; }
     }
     else
     {
       if($inRslFilename)
       { die "Too many arguments.  Usage: perl mpi2gur.pl [flags] <RSL filename>\n"; }
       else
       { $inRslFilename = $ARGV[$i]; }
     }
  }

  if(!$inRslFilename)
  { die "usage: mpi2gur [flags] <RSL filename>\n"; }

  print "Using input file $inRslFilename\n";
  print "Run stage is $runStage\n";
  print "Wait for reservation is $waitForRes\n";

}  # parseArguments

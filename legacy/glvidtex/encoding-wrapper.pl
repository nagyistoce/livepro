#!/usr/bin/perl
use strict;

use Data::Dumper;

my $VLOOPBACK_KO 	= 'vloopback/vloopback.ko';
my $RESIZE_BIN 		= 'vloopback/example/resize';
my $ENCODER_STATE_FILE	= '/var/run/glstreamenc.dat';

my $StreamerPID = 0;
my $ResizePID = 0;
my $OutputFile = undef;
my $StreamerFile = undef;

# Populated by insert_vloopback()
# Used by start_resize() and start_streamer() 
my @PipePairs = ();

main();
exit;

##########

sub main
{
	my $action = shift @ARGV 
		|| die "Usage: $0 (vloopon|vloopoff|encodeon|encodeoff) [output file] [src size] [dest size]\n";

	load_encoder_state();
	
	if($action eq 'vloopon')
	{
		insert_vloopback();
		
		print $PipePairs[0]->{in},  "\n";
	}
	elsif($action eq 'vloopoff')
	{
		stop_encoding() if $StreamerPID;
		
		my $result = `rmmod vloopback 2>&1`;
		if($result =~ /ERROR/)
		{
			print STDERR "Unable to remove vloopback kernel module: $result";
		}
		else
		{
			@PipePairs = ();
		}
	}
	elsif($action eq 'encodeon')
	{
		if(!@PipePairs)
		{
			print "$0: Unable to start encoding because the vloopback module has not been inserted by this script - therefore, the state data was not recorded and likely glvidtex is not outputting to the vloopback pipe anyway.";
			exit -1;
		}
		
		if(!$StreamerPID)
		{
			$OutputFile   = shift @ARGV || die "Usage: $0 encodeon <output file> [src size] [dest size]\n";
			$StreamerFile = $OutputFile;
			
			my $source_size = shift @ARGV || '1024x768';
			my $dest_size   = shift @ARGV || '640x480';
			
			my $audio_config = lc shift @ARGV ne 'noaudio' ? '-F stereo' : '';
			
				
			if($OutputFile =~ /\.mpg$/i)
			{
				$StreamerFile =~ s/\.mpg$/.avi/i;
			}
			
			# need starting res (from player) and destination res - only start if neede
			# If already running, ... ???
			#start_resize($source_size, $dest_size, $PipePairs[0]->{out}, $PipePairs[1]->{out});
			my $streamer_input = $PipePairs[0]->{out};
			if($source_size ne $dest_size)
			{
				if(!fork)
				{
					system("$RESIZE_BIN $PipePairs[0]->{out} $PipePairs[1]->{in} $source_size $dest_size rgb24 2>&1 1>/dev/null");
				}
				
				sleep 1;
				
				my $pid = `pidof $RESIZE_BIN`;
				$pid =~ s/[\r\n]//g;
				$ResizePID = $pid;
				
				$streamer_input = $PipePairs[1]->{out};
			}
			
			if(!fork)
			{
				system("streamer -c $streamer_input -s $dest_size -r 24 -o $StreamerFile -f jpeg $audio_config -t 3:00:00 2>&1 1>/dev/null");
			}
			
			sleep 1;
			
			my $pid = `pidof streamer`;
			$pid =~ s/[\r\n]//g;
			$StreamerPID = $pid;
			
		}
	}	
	elsif($action eq 'encodeoff')
	{
		stop_encoding();
	}
	
	save_encoder_state();
}

sub load_encoder_state()
{
	my @data = `cat $ENCODER_STATE_FILE`;
	s/[\r\n]//g foreach @data;
	
	$StreamerPID	= shift @data;
	$ResizePID	= shift @data;
	$OutputFile	= shift @data;
	$StreamerFile	= shift @data;
	
	foreach my $line (@data)
	{
		my ($in,$out) = split /\|/, $line;
		push @PipePairs, { in=>$in, out=>$out };	
	}
}

sub save_encoder_state()
{
	open(FILE, ">$ENCODER_STATE_FILE") || die "Cannot save encoder state - unable to open $ENCODER_STATE_FILE for writing: $!";
	print FILE $StreamerPID,  "\n";
	print FILE $ResizePID,    "\n";
	print FILE $OutputFile,   "\n";
	print FILE $StreamerFile, "\n";
	foreach my $pair (@PipePairs)
	{
		print FILE "$pair->{in}|$pair->{out}\n";
	}
	close(FILE);
}

sub insert_vloopback
{
	# Need to know path to vloopback.ko file
	
	my @check = grep /^vloopback/, split /\n/, `lsmod | grep vloopback`;
	if(@check)
	{
		# Vloopback already installed
		
		#print "insert_vloopback: Already inserted, not reinserting\n";
		#print Dumper \@check;
	}
	else
	{
		@PipePairs = ();
		
		my $response = `insmod $VLOOPBACK_KO pipes=2 2>&1`;
		if($response =~ /No such file/)
		{
			print STDERR "insert_vloopback: Error loading vloopback module: $response";
		}
		else
		{
			my @pipes = split /\n/, `dmesg | tail -n 6 | grep 'registered, input'`;
			foreach my $line (@pipes)
			{
				my ($p_in, $p_out) = $line =~ /(video\d+).*(video\d+)/;
				push @PipePairs, { in => "/dev/$p_in", out => "/dev/$p_out" }; 
			}
			
			#print STDERR "insert_vloopback: Kernel module loaded, created ".scalar(@PipePairs)." pipe pairs.\n";
		}
	}
}

sub stop_encoding
{
	system("kill $StreamerPID");
	$StreamerPID = 0;
	
	system("kill $ResizePID");
	$ResizePID= 0;
	
	if(-f $StreamerFile && $StreamerFile ne $OutputFile)
	{
		# If we dont remove an existing output file, ffmpeg will prompt for overwrite on stdin
		unlink($OutputFile);
		
		system("ffmpeg -i $StreamerFile -b 2000000 -ab 256 $OutputFile");
		
		# Remove the temporary streamer file, leaving only $OutputFile
		unlink($StreamerFile);
	}
}


#
# $Id$
#
# Copyright (C) 1998 Tuomas J. Lukka, 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# This file hadles URLs for FreeWRL -
# the idea is to make it possible to do without libwww-perl
# (I don't know if the emulation really works...)
# 

package VRML::URL;
use strict;
use vars qw/$has_lwp/;

# See if we have libwww-perl

unless($VRML::ENV{FREEWRL_NO_LWP})  {

	eval 'require LWP::Simple; require URI::URL;';
	if($@) {
		warn "You don't seem to have libwww-perl installed.
	Cannot get files from the net";
	} else {
		$has_lwp = 1;
	}

}

##############################################################
#
# We make it possible to save stuff 

%VRML::URL::savedUrls;
my @urlKeyList = qw(
		    AbsolutePath
		    FileContents
		    UncachedParentUrl
		   );

{
my $ind = 0;


sub save_file {
	my ($url, $file_contents, $key) = @_;
	my $s;
	if($s = $VRML::ENV{FREEWRL_SAVE}) {
		system("cp $file_contents $s/s$ind");
		system(qq{echo "$url -> $s/s$ind" >>$s/dir});
		$ind ++;
	}
	$VRML::URL::savedUrls{$key}{$urlKeyList[1]} = $file_contents;
	return $file_contents;
}

sub save_text {
	my ($url, $text, $key) = @_;
	my $s;
	if($s = $VRML::ENV{FREEWRL_SAVE}) {
		open FOO, ">$s/s$ind";
		print FOO $text;
		close FOO;
		system(qq{echo "$url -> $s/s$ind" >>$s/dir});
		$ind ++;
	}
	$VRML::URL::savedUrls{$key}{$urlKeyList[1]} = $text;
	return $text;
}

}

##############################################################
#
# Much of the VRML content is gzipped -- we have to recognize
# it in the run.

sub is_gzip {
	if($_[0] =~ /^\037\213/) {
		# warn "GZIPPED content -- trying to ungzip\n";
		return 1;
	}
	return 0;
}

sub ungzip_file {
	my($file) = @_;
	if($file !~ /^[-\w~\.,\/]+$/) {
	 warn("Suspicious file name '$file' -- not gunzipping");
	 return $file;
	}
	open URLFOO,"<$file";
	my $a;
	read URLFOO, $a, 10;
	if(is_gzip($a)) {
		print "Seems to be gzipped - ungzipping\n" if $VRML::verbose::url;
		my $f = temp_file();
		system("gunzip <$file >$f") == 0
		 or die("Gunzip failed: $?");
		return $f;
	} else {
		return $file;
	}
	close URLFOO;
}

sub ungzip_text {
	if(is_gzip($_[0])) {
		my $f = temp_file();
		open URLBAR, "|gunzip >$f";
		local $SIG{PIPE} = sub { die("Gunzip pipe broke"); };
		print URLBAR $_[0];
		close URLBAR || die("bad Gunzip: $! $?");;
		open URLFOO, "<$f";
		my $t = join '',<URLFOO>;
		close URLFOO;
		return $t;
	}
	return $_[0];
}

sub get_really {
	my ($url) = @_;
	$url = URI::URL::url($url,"file:".getcwd()."/")->abs->as_string;
	print "VRML::URL::really '$url'\n" if $VRML::verbose::url;
	return $url;
}

# Changes to VRML::URL::get_absolute:
#
# If a url can't be found, a warning is issued and an undefined 
# value returned. The caller is required to handle the failure to
# locate the url.
# The reason for the change is to bring FreeWRL's url handling behaviour
# in compliance with the VRML97 specification (refer to the section
# entitled "VRML and the World Wide Web" and bug 435578).
#
# Previously, the function exited by calling die if a url could not
# be found.

sub get_absolute {
    my($url, $as_file, $key) = @_;
    my $local_filepath = 0;

    if (!$key) {
		$key = ":$url";
	}

    ## TODO : Save modification date of file and check for changes
    if ($VRML::URL::savedUrls{$key}{$urlKeyList[1]} && !$main::sig_reload) {
		return $VRML::URL::savedUrls{$key}{$urlKeyList[1]};
    }

    print "VRML::URL::get_absolute('$url', $as_file, $key)\n" if $VRML::verbose::url;
    if ($has_lwp) {
		use POSIX qw/getcwd/;

		if ($url =~ /^file:(.*)$/) {
			$local_filepath = 1;
		} else {
			$url = get_really($url);
		}

		if (!$as_file) {
			my $r = LWP::Simple::get($url);
			if (!$r) {
				warn("Url $url not obtained... something is wrong");
				return undef;
			}

			print "LWP::Simple::get returned ".length($r)." bytes\n"
				if $VRML::verbose::url;
			return save_text($url, ungzip_text($r), $key);
		} else {
			if ($local_filepath) {
				if (-e $1) {
					return save_file($url, ungzip_file($1), $key);
				} else {
					warn("Local file $1 not obtained... something is wrong");
					return undef;
				}
			} else {
				my($name) = temp_file();
				my $rc = LWP::Simple::getstore($url, $name);
				if (LWP::Simple::is_success($rc)) {
					return save_file($url, ungzip_file($name), $key);
				} else {
					warn("Url $url not obtained as file... something is wrong");
					return undef;
				}
			}
		}
    } else {
		if (-e $url) {
			$url = ungzip_file($url);
			if ($as_file) {
				return save_file($url, $url, $key);
			}
			open FOOFILE, "<$url";
			my $str = join '',<FOOFILE>;
			close FOOFILE;
			return save_text($url, $str, $key);
	    } else {
			warn("Cannot find '$url' -- if it is a web address, you need to install libwww-perl");
			return undef;
	    }
    }
}

# Changes to VRML::URL::get_relative similar to
# those made to VRML::URL::get_absolute:

sub get_relative {
    my($base, $file, $as_file) = @_;
    $base = get_really($base);
    print "VRML::URL::get_relative('$base', '$file', $as_file)\n" if $VRML::verbose::url;

    my $key = "$base:$file";
    my $url;

    if (!$VRML::URL::savedUrls{$key}{$urlKeyList[0]}) {
		if ($VRML::ENV{AS_PLUGIN}) {
			eval 'require VRML::PluginGlue';

			$url = VRML::PluginGlue::requestUrl($VRML::PluginGlue::globals{pluginSock},
												$VRML::PluginGlue::globals{instance},
												$file);
			if (!$url) {
				warn("Web browser plugin could not retrieve $file.\n");
				return undef;
			}
		} elsif ($has_lwp) {
			$url = URI::URL::url($file, $base)->abs->as_string;
		} else {
			$url = $base;
			$url =~ s/[^\/]+$/$file/ or die("Can't do relativization");
		}

		$VRML::URL::savedUrls{$key} = {
									   $urlKeyList[0] => $url, # AbsolutePath
									  };
    } else {
		$url = $VRML::URL::savedUrls{$key}{$urlKeyList[0]};
    }

    my $txt = get_absolute($url, $as_file, $key);

    return (wantarray ? ($txt, $url) : $txt);
}

# Taken from perlfaq5
{
my %temps;
BEGIN {
	use IO::File;
	use Fcntl;
	my $temp_dir = -d '/tmp' ? '/tmp' : $ENV{TMP} || $ENV{TEMP};
	my $base_name = sprintf("%s/freewrl-%d-%d-00000", $temp_dir, $$, time());
	sub temp_file {
	   my $fh = undef;
	   my $count = 0;
	   until (defined($fh) || $count > 100) {
	       $base_name =~ s/-(\d+)$/"-" . (1 + $1)/e;
	       $temps{$base_name} = 1;
	       $fh = IO::File->new($base_name, O_WRONLY|O_EXCL|O_CREAT,0644)
	   }
	   if (defined($fh)) {
	       undef $fh;
	       unlink $base_name;
	       return $base_name;
	   } else {
	       die("Couldn't make temp file");
	   }
	}
}

sub unlinktmp {
	if(!$temps{$_[0]}) {
		die("Trying to unlink nonexistent tmp");
	}
	unlink $_[0];
	delete $temps{$_[0]};
}
END {
	for(keys %temps) {
		print "unlinking '$_' (NOT)\n" if $VRML::verbose::url;
		# unlink $_;
	}
}
}

1;

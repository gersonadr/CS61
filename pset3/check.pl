#! /usr/bin/perl -w

# check.pl
#    This program runs the tests in io61 and stdio versions.
#    It compares their outputs and measures time and memory usage.
#    It tries to prevent disaster: if your code looks like it's
#    generating an infinite-length file, or using too much memory,
#    check.pl will kill it.
#
#    To add tests of your own, scroll down to the bottom. It should
#    be relatively clear what to do.

use Time::HiRes qw(gettimeofday);
use Fcntl qw(F_GETFL F_SETFL O_NONBLOCK);
use POSIX;
use Scalar::Util qw(looks_like_number);
my($nkilled) = 0;
my($nerror) = 0;
my(@ratios, @runtimes, @basetimes, @alltests);
my(%fileinfo);
my($NOSTDIO) = exists($ENV{"NOSTDIO"});
my($NOYOURCODE) = exists($ENV{"NOYOURCODE"});
my($TRIALTIME) = exists($ENV{"TRIALTIME"}) ? $ENV{"TRIALTIME"} + 0 : 3;
my($TRIALS) = exists($ENV{"TRIALS"}) ? int($ENV{"TRIALS"}) : 5;
$TRIALS = 5 if $TRIALS <= 0;
my($STDIOTRIALS) = exists($ENV{"STDIOTRIALS"}) ? int($ENV{"STDIOTRIALS"}) : $TRIALS;
$STDIOTRIALS = $TRIALS if $STDIOTRIALS <= 0;
my($MAXTIME) = exists($ENV{"MAXTIME"}) ? $ENV{"MAXTIME"} + 0 : 20;
$MAXTIME = 20 if $MAXTIME <= 0;
sub first (@) { return $_[0]; }
my($CHECKSUM) = first(grep {-x $_} ("/usr/bin/md5sum", "/sbin/md5",
                                    "/bin/false"));
my($VERBOSE) = exists($ENV{"VERBOSE"});
my($MAKE) = exists($ENV{"MAKE"}) && int($ENV{"MAKE"});
eval { require "syscall.ph" };

my($Red, $Redctx, $Green, $Cyan, $Off) = ("\x1b[01;31m", "\x1b[0;31m", "\x1b[01;32m", "\x1b[01;36m", "\x1b[0m");
$Red = $Redctx = $Green = $Cyan = $Off = "" if !-t STDERR || !-t STDOUT;


$SIG{"CHLD"} = sub {};
my($run61_pid);

sub decache ($) {
    my($fn) = @_;
    if (defined(&{"SYS_fadvise64"}) && open(DECACHE, "<", $fn)) {
        syscall &SYS_fadvise64, fileno(DECACHE), 0, -s DECACHE, 4;
        close(DECACHE);
    }
}

sub makefile ($$) {
    my($filename, $size) = @_;
    if (!-r $filename || !defined(-s $filename) || -s $filename != $size) {
        while (!defined(-s $filename) || -s $filename < $size) {
            system("cat /usr/share/dict/words >> $filename");
        }
        truncate($filename, $size);
    }
    $fileinfo{$filename} = [-M $filename, -C $filename, $size];
}

sub makebinaryfile ($$) {
    my($filename, $size) = @_;
    if (!-r $filename || !defined(-s $filename) || -s $filename != $size) {
        while (!defined(-s $filename) || -s $filename < $size) {
            system("cat /bin/sh >> $filename");
        }
        truncate($filename, $size);
    }
    $fileinfo{$filename} = [-M $filename, -C $filename, $size];
}

sub verify_file ($) {
    my($filename) = @_;
    if (exists($fileinfo{$filename})
        && ($fileinfo{$filename}->[0] != -M $filename
            || $fileinfo{$filename}->[1] != -C $filename)) {
        truncate($filename, 0);
        if ($filename =~ /^binary/) {
            makebinaryfile($filename, $fileinfo{$filename}->[2]);
        } else {
            makefile($filename, $fileinfo{$filename}->[2]);
        }
    }
    return -s $filename;
}

sub file_md5sum ($) {
    my($x) = `$CHECKSUM $_[0]`;
    $x =~ s{\A(\S+).*\z}{$1}s;
    return $x;
}

sub run_sh61_pipe ($$;$) {
    my($text, $fd, $size) = @_;
    my($n, $buf) = (0, "");
    return $text if !defined($fd);
    while ((!defined($size) || length($text) <= $size)
           && defined(($n = POSIX::read($fd, $buf, 8192)))
           && $n > 0) {
        $text .= substr($buf, 0, $n);
    }
    return $text;
}

sub run_sh61 ($;%) {
    my($command, %opt) = @_;
    my($outfile) = exists($opt{"stdout"}) ? $opt{"stdout"} : undef;
    my($size_limit_file) = exists($opt{"size_limit_file"}) ? $opt{"size_limit_file"} : $outfile;
    $size_limit_file = [$size_limit_file] if $size_limit_file && !ref($size_limit_file);
    my($size_limit) = exists($opt{"size_limit"}) ? $opt{"size_limit"} : undef;
    my($dir) = exists($opt{"dir"}) ? $opt{"dir"} : undef;
    if (defined($dir) && $size_limit_file) {
        $dir =~ s{/+$}{};
        $size_limit_file = [map { m{^/} ? $_ : "$dir/$_" } @$size_limit_file];
    }
    pipe(PR, PW);
    pipe(OR, OW);
    fcntl(OR, F_SETFL, fcntl(OR, F_GETFL, 0) | O_NONBLOCK);
    1 while waitpid(-1, WNOHANG) > 0;

    $run61_pid = fork();
    if ($run61_pid == 0) {
        setpgrp(0, 0);
        defined($dir) && chdir($dir);

        close(PR);
        POSIX::dup2(fileno(PW), 100);
        close(PW);

        close(OR);
        POSIX::dup2(fileno(OW), 1);
        POSIX::dup2(fileno(OW), 2);
        close(OW);

        { exec($command) };
        print STDERR "error trying to run $command: $!\n";
        exit(1);
    }

    my($before) = Time::HiRes::time();
    my($died) = 0;
    my($max_time) = exists($opt{"time_limit"}) ? $opt{"time_limit"} : 0;
    my($out, $buf, $nb) = ("", "");

    close(PW);
    close(OW);

    eval {
        do {
            Time::HiRes::usleep(300000);
            die "!" if waitpid($run61_pid, WNOHANG) > 0;
            if (defined($size_limit) && $size_limit_file && @$size_limit_file) {
                my($len) = 0;
                run_sh61_pipe($out, fileno(OR), $size_limit);
                foreach my $fname (@$size_limit_file) {
                    $len += ($fname eq "pipe" ? length($out) : -s $fname);
                }
                if ($len > $size_limit) {
                    $died = "output file size too large, expected <= $size_limit";
                    die "!";
                }
            }
        } while (Time::HiRes::time() < $before + $max_time);
        $died = sprintf("timeout after %.2fs", $max_time)
            if waitpid($run61_pid, WNOHANG) <= 0;
    };

    my($delta) = Time::HiRes::time() - $before;

    my($answer) = exists($opt{"answer"}) ? $opt{"answer"} : {};
    $answer->{"command"} = $command;
    $answer->{"time"} = $delta;

    if (exists($opt{"nokill"})) {
        $answer->{"pgrp"} = $run61_pid;
    } else {
        kill 9, -$run61_pid;
    }
    $run61_pid = 0;

    if ($died) {
        $answer->{"error"} = $died;
        close(OR);
        close(PR);
        return $answer;
    }

    $nb = POSIX::read(fileno(PR), $buf, 2000);
    close(PR);
    $buf = $nb > 0 ? substr($buf, 0, $nb) : "";

    while ($buf =~ m,\"(.*?)\"\s*:\s*([\d.]+),g) {
        $answer->{$1} = $2;
    }
    $answer->{"time"} = $delta if !defined($answer->{"time"});
    $answer->{"time"} = $delta if $answer->{"time"} <= 0.95 * $delta;
    $answer->{"utime"} = $delta if !defined($answer->{"utime"});
    $answer->{"stime"} = $delta if !defined($answer->{"stime"});
    $answer->{"maxrss"} = -1 if !defined($answer->{"maxrss"});

    run_sh61_pipe($out, fileno(OR), $size_limit);
    close(OR);

    if ($size_limit_file && @$size_limit_file) {
        my($len, @sums) = 0;
        foreach my $fname (@$size_limit_file) {
            $len += ($fname eq "pipe" ? length($out) : -s $fname);
            if ($VERBOSE && $fname eq "pipe") {
                # XXX
            } elsif (($VERBOSE || exists($ENV{"MAKETRIALLOG"}) || exists($ENV{"TRIALLOG"}))
                     && -f $fname
                     && (!exists($opt{"no_content_check"}) || !$opt{"no_content_check"})) {
                push @sums, file_md5sum($fname);
            }
        }
        $answer->{"outputsize"} = $len;
        $answer->{"md5sum"} = join(" ", @sums) if @sums;
    }

    if ($out) {
        my($tx) = "";
        foreach my $l (split(/\n/, $out)) {
            $tx .= ($tx eq "" ? "" : "        : ") . $l . "\n" if $l ne "";
        }
        if ($tx ne "" && exists($answer->{"trial"})) {
            $answer->{"stderr"} = "    ${Redctx}YOUR STDERR (TRIAL " . $answer->{"trial"} . "): $tx${Off}";
        } elsif ($tx ne "") {
            $answer->{"stderr"} = "    ${Redctx}YOUR STDERR: $tx${Off}";
        }
    }

    return $answer;
}

sub read_triallog ($) {
    my($buf);
    open(TRIALLOG, "<", $_[0]) or die "$_[0]: $!\n";
    while (defined($buf = <TRIALLOG>)) {
        my($t) = {};
        while ($buf =~ m,"([^"]*)"\s*:\s*([\d.]+),g) {
            $t->{$1} = $2 + 0;
        }
        while ($buf =~ m,"([^"]*)"\s*:\s*"([^"]*)",g) {
            $t->{$1} = $2;
        }
        push @alltests, $t if keys(%$t);
    }
    close(TRIALLOG);
}

sub maybe_make ($) {
    my($command) = @_;
    if ($MAKE && $command =~ m<(?:^|[|&;]\s*)./(\S+)>) {
        $verbose = defined($ENV{"V"}) && $ENV{"V"} && $ENV{"V"} ne "0";
        if (system($verbose ? "make $1" : "make -s $1") != 0) {
            print STDERR "${Red}ERROR: Cannot make $1${Off}\n";
            exit 1;
        }
    }
}

sub run_trials ($$$$$$$%) {
    my($number, $type, $command, $infiles, $outfiles, $max_size, $max_trials, %opt) = @_;
    my($ntrials, $nerrors, $totaltime) = (0, 0, 0);

    do {
        foreach my $f (@$infiles) {
            decache($f);
        }
        Time::HiRes::usleep(100000);

        my($t) = run_sh61($command,
                          "size_limit_file" => $outfiles,
                          "time_limit" => ($type eq "stdio" ? 60 : $MAXTIME),
                          "size_limit" => $max_size,
                          "answer" => {"number" => $number,
                                       "type" => $type,
                                       "trial" => $ntrials + 1},
                          "no_content_check" => exists($opt{"no_content_check"}));
        push @alltests, $t;

        $ntrials += 1;
        $nerrors += 1 if exists($t->{"error"});
        $totaltime += $t->{"time"};
    } while ($ntrials < $max_trials
             && ($TRIALTIME <= 0 || $totaltime < $TRIALTIME)
             && $nerrors <= 1);
}

sub median_trial ($$$;$) {
    my($number, $type, $command, $tcompar) = @_;
    my(@tests) = grep {
        $_->{"number"} == $number && $_->{"type"} eq $type
            && $_->{"command"} eq $command
    } @alltests;
    return undef if !@tests;

    # return error test if more than one error observed
    my(@errortests) = grep { exists($_->{"error"}) } @tests;
    if (@errortests > 1) {
        $errortests[0]->{"medianof"} = scalar(@tests);
        $errortests[0]->{"stderr"} = "" if !exists($errortests[0]->{"stderr"});
        return $errortests[0];
    }

    # collect stderr and md5sum from all tests
    my($stderr) = join("", map {
                           exists($_->{"stderr"}) ? $_->{"stderr"} : ""
                       } @tests);
    my(%md5sums) = map {
        exists($_->{"md5sum"}) ? ($_->{"md5sum"} => 1) : ()
    } @tests;
    my(%outputsizes) = map {
        exists($_->{"outputsize"}) ? ($_->{"outputsize"} => 1) : ()
    } @tests;

    # pick median test (or another test that's not erroneous)
    @tests = sort { $a->{"time"} <=> $b->{"time"} } @tests;
    my $tt = {};
    my $tidx = int(@tests / 2);
    for (my $j = 0; $j < @tests; ++$j) {
        %$tt = %{$tests[$tidx]};

        if ($tcompar) {
            if (exists($tcompar->{"outputsize"})
                && exists($tt->{"outputsize"})
                && $tt->{"outputsize"} != $tcompar->{"outputsize"}) {
                $tt->{"different_size"} = 1;
            }
            if (exists($tcompar->{"content_check"})) {
                foreach my $fname (@{$tcompar->{"content_check"}}) {
                    my($basefname) = $fname;
                    $basefname =~ s{files/}{files/base};
                    my($r) = scalar(`cmp $basefname $fname 2>&1`);
                    chomp $r if $r;
                    $r =~ s/^cmp: // if $r;
                    $tt->{"different_content"} = " ($r)" if $?;
                }
            }
            if (exists($tcompar->{"md5sum_check"}) && exists($tt->{"md5sum"})) {
                $tt->{"different_content"} = " (got md5sum " . $tt->{"md5sum"}
                    . ", expected " . $tcompar->{"md5sum_check"} . ")"
                    if $tcompar->{"md5sum_check"} ne $tt->{"md5sum"};
            }
        }

        last if !exists($tt->{"error"}) && !exists($tt->{"different_size"})
            && !exists($tt->{"different_content"});

        $tidx = ($tidx + 1) % @tests;
    }

    # decorate it
    $tt->{"medianof"} = scalar(@tests);
    $tt->{"stderr"} = $stderr;
    if (keys(%md5sums) == 1) {
        $tt->{"md5sum"} = (keys(%md5sums))[0];
    }
    if (keys(%outputsizes) > 1 || keys(%md5sums) > 1) {
        $tt->{"stderr"} .= "           ${Red}ERROR: trial runs generated different output${Off}\n";
    }
    return $tt;
}

sub run ($$$%) {
    my($number, $command, $desc, %opt) = @_;
    return if (@ARGV && !grep {
        ($_ =~ m{^\s*(?:0b[01]+|0[0-7]*|0x[0-9a-fA-F]+|[0-9]*)\s*$}
         && $_ == $number)
            || ($_ =~ m{^(\d+)-(\d+)$} && $number >= $1 && $number <= $2)
            || ($_ =~ m{(?:^|,)$number(,|$)})
               } @ARGV);
    my($expansion) = $opt{"expansion"};
    $expansion = 1 if !$expansion;

    # verify input files, print header
    my(@infiles);
    my($insize) = 0;
    foreach my $infile (keys %fileinfo) {
        if ($command =~ /\b$infile\b/) {
            push @infiles, $infile;
            $insize += verify_file($infile);
        }
    }
    my($outsize) = $expansion * $insize;
    print "TEST:      $number. $desc\n";
    print "COMMAND:   $command\n" if !exists($ENV{"NOCOMMAND"});
    my($outsuf) = ".txt";
    $outsuf = ".bin" if $command =~ m<out\.bin>;

    # run stdio version
    my($stdiocmd) = $command;
    $stdiocmd =~ s<(\./)([a-z]*61)><${1}stdio-$2>g;
    $stdiocmd =~ s<out(\d*)\.(txt|bin)><baseout$1\.$2>g;
    my(@outfiles) = ();
    while ($stdiocmd =~ m{([^\s<>]*baseout\d*\.(?:txt|bin))}g) {
        push @outfiles, $1;
    }
    if (!$NOSTDIO) {
        maybe_make($stdiocmd);
        print "STDIO:     ";
        run_trials($number, "stdio", $stdiocmd, \@infiles,
                   \@outfiles, undef, $STDIOTRIALS, %opt);
    }
    my($t) = median_trial($number, "stdio", $stdiocmd);
    print "STDIO:     " if $t && $NOSTDIO;
    if ($t) {
        if (exists($t->{"utime"})) {
            printf("%.5fs (%.5fs user, %.5fs system, %dKiB memory, %d trial%s)\n",
                   $t->{"time"}, $t->{"utime"}, $t->{"stime"}, $t->{"maxrss"},
                   $t->{"medianof"}, $t->{"medianof"} == 1 ? "" : "s");
        } else {
            printf("${Red}KILLED${Redctx} after %.5fs (%d trial%s)${Off}\n",
                   $t->{"time"},
                   $t->{"medianof"}, $t->{"medianof"} == 1 ? "" : "s");
        }
        if (exists($t->{"outputsize"}) && $t->{"outputsize"} > $outsize) {
            $outsize = $t->{"outputsize"};
        }
    }

    # run yourcode version
    @outfiles = ();
    while ($command =~ m{([^\s<>]*out\d*\.(?:txt|bin))}g) {
        push @outfiles, $1;
    }
    if (!$NOYOURCODE) {
        maybe_make($command);
        print "YOUR CODE: ";
        run_trials($number, "yourcode", $command, \@infiles,
                   \@outfiles, $outsize * 2, $TRIALS, %opt);
    }
    my($tcompar) = {};
    $tcompar->{"outputsize"} = $t->{"outputsize"}
        if exists($t->{"outputsize"});
    $tcompar->{"content_check"} = \@outfiles
        if !$NOYOURCODE && !$opt{"no_content_check"} && !$NOSTDIO;
    $tcompar->{"md5sum_check"} = $t->{"md5sum"}
        if !$NOYOURCODE && !$opt{"no_content_check"} && $NOSTDIO && exists($t->{"md5sum"});
    my($tt) = median_trial($number, "yourcode", $command, $tcompar);
    print "YOUR CODE: " if $tt && $NOYOURCODE;
    if ($tt && defined($tt->{"error"})) {
        printf "${Red}KILLED${Redctx} (%s)${Off}\n", $tt->{"error"};
        ++$nkilled;
    } elsif ($tt) {
        printf("%.5fs (%.5fs user, %.5fs system, %dKiB memory, %d trial%s)\n",
               $tt->{"time"}, $tt->{"utime"}, $tt->{"stime"}, $tt->{"maxrss"},
               $tt->{"medianof"}, $tt->{"medianof"} == 1 ? "" : "s");
        push @runtimes, $tt->{"time"};
    }

    # print stdio vs. yourcode comparison
    if ($t && $tt && $tt->{"time"} && !defined($tt->{"error"})
        && !defined($tt->{"different_size"})
        && !defined($tt->{"different_content"})) {
        my($ratio) = $t->{"time"} / $tt->{"time"};
        my($color) = ($ratio < 0.5 ? $Redctx : ($ratio > 1.9 ? $Green : $Cyan));
        printf("RATIO:     ${color}%.2fx stdio${Off}\n", $ratio);
        push @ratios, $ratio;
        push @basetimes, $t->{"time"};
    }
    if (exists($tt->{"different_size"})) {
        print "           ${Red}ERROR: ", join("+", @outfiles), " has size ",
            $tt->{"outputsize"}, ", expected ", $t->{"outputsize"}, "${Off}\n";
    }
    if (exists($tt->{"different_content"})) {
        my(@xoutfiles) = map {s{^files/}{}; $_} @outfiles;
        print "           ${Red}ERROR: ", join("+", @xoutfiles),
            " differs from stdio's ", join("+", map {"base$_"} @xoutfiles),
            "${Redctx}", $tt->{"different_content"}, "$Off\n";
        ++$nerror;
    }

    # print yourcode stderr and a blank-line separator
    print $tt->{"stderr"} if exists($tt->{"stderr"}) && $tt->{"stderr"} ne "";
    print "\n";
}

sub pl ($$) {
    my($n, $x) = @_;
    return $n . " " . ($n == 1 ? $x : $x . "s");
}

sub summary () {
    my($ntests) = @runtimes + $nkilled;
    print "SUMMARY:   ", pl($ntests, "test"), ", ";
    if ($nkilled) {
        print "${Red}$nkilled killed,${Off} ";
    } else {
        print "0 killed, ";
    }
    if ($nerror) {
        print "${Red}", pl($nerror, "error"), "${Off}\n";
    } else {
        print "0 errors\n";
    }
    my($better) = scalar(grep { $_ > 1 } @ratios);
    my($worse) = scalar(grep { $_ < 1 } @ratios);
    if ($better || $worse) {
        print "           better than stdio ", pl($better, "time"),
        ", worse ", pl($worse, "time"), "\n";
    }
    my($mean, $basetime, $runtime) = (0, 0, 0);
    for (my $i = 0; $i < @ratios; ++$i) {
        $mean += $ratios[$i];
        $basetime += $basetimes[$i];
    }
    for (my $i = 0; $i < @runtimes; ++$i) {
        $runtime += $runtimes[$i];
    }
    if (@ratios) {
        printf "           average %.2fx stdio\n", $mean / @ratios;
        printf "           total time %.3fs stdio, %.3fs your code (%.2fx stdio)\n",
        $basetime, $runtime, $basetime / $runtime;
    } elsif (@runtimes) {
        printf "           total time %.3f your code\n", $runtime;
    }

    if ($VERBOSE || exists($ENV{"MAKETRIALLOG"})) {
        my(@testjsons);
        foreach my $t (@alltests) {
            my(@tout, $k, $v) = ();
            while (($k, $v) = each %$t) {
                push @tout, "\"$k\":" . (looks_like_number($v) ? $v : "\"$v\"");
            }
            push @testjsons, "{" . join(",", @tout) . "}\n";
        }
        print "\n", @testjsons if $VERBOSE;
        if (exists($ENV{"MAKETRIALLOG"}) && $ENV{"MAKETRIALLOG"}) {
            open(OTRIALLOG, ">", $ENV{"MAKETRIALLOG"} eq "1" ? "triallog.txt" : $ENV{"MAKETRIALLOG"}) or die;
            print OTRIALLOG @testjsons;
            close(OTRIALLOG);
        }
    }
}

# maybe read a trial log
if (exists($ENV{"TRIALLOG"})) {
    read_triallog($ENV{"TRIALLOG"});
}

# create some files
if (!-d "files" && (-e "files" || !mkdir("files"))) {
    print STDERR "*** Cannot run tests because 'files' cannot be created.\n";
    print STDERR "*** Remove 'files' and try again.\n";
    exit(1);
}
makefile("files/text1meg.txt", 1 << 20);
makebinaryfile("files/binary1meg.bin", 1 << 20);
makefile("files/text5meg.txt", 5 << 20);
makefile("files/text20meg.txt", 20 << 20);

$SIG{"INT"} = sub {
    kill 9, -$run61_pid if $run61_pid;
    summary();
    exit(1);
};


# REGULAR FILES, SEQUENTIAL I/O

run(1,
    "./cat61 files/text1meg.txt > files/out.txt",
    "regular small file, character I/O, sequential");

run(2,
    "./cat61 files/binary1meg.bin > files/out.bin",
    "regular small binary file, character I/O, sequential");

run(3,
    "./cat61 files/text20meg.txt > files/out.txt",
    "regular large file, character I/O, sequential");

run(4,
    "./blockcat61 -b 1024 files/text5meg.txt > files/out.txt",
    "regular medium file, 1KB block I/O, sequential");

run(5,
    "./blockcat61 files/text20meg.txt > files/out.txt",
    "regular large file, 4KB block I/O, sequential");

run(6,
    "./blockcat61 files/binary1meg.bin > files/out.bin",
    "regular small binary file, 4KB block I/O, sequential");

run(7,
    "./randblockcat61 files/text20meg.txt > files/out.txt",
    "regular large file, 1B-4KB block I/O, sequential");

run(8,
    "./randblockcat61 -r 6582 files/text20meg.txt > files/out.txt",
    "regular large file, 1B-4KB block I/O, sequential");


# MULTIPLE REGULAR FILES (mostly a correctness test)

run(9,
    "./gather61 -b 512 files/binary1meg.bin files/text1meg.txt > files/out.bin",
    "gathered small files, 512B block I/O, sequential");

run(10,
    "./scatter61 -b 512 files/out1.txt files/out2.txt files/out3.txt < files/text1meg.txt",
    "scattered small file, 512B block I/O, sequential");


# REGULAR FILES, REVERSE I/O

run(11,
    "./reverse61 files/text5meg.txt > files/out.txt",
    "regular medium file, character I/O, reverse order");

run(12,
    "./reverse61 files/text20meg.txt > files/out.txt",
    "regular large file, character I/O, reverse order");


# FUNNY FILES

run(13,
    "./cat61 -s 4096 /dev/urandom > files/out.txt",
    "seekable unmappable file, character I/O, sequential",
    "no_content_check" => 1);

run(14,
    "./reverse61 -s 4096 /dev/urandom > files/out.txt",
    "seekable unmappable file, character I/O, reverse order",
    "no_content_check" => 1);

run(15,
    "./cat61 -s 5242880 /dev/zero > files/out.txt",
    "magic zero file, character I/O, sequential");

run(16,
    "./reverse61 -s 5242880 /dev/zero > files/out.txt",
    "magic zero file, character I/O, reverse order");


# STRIDE AND REORDER I/O PATTERNS

run(17,
    "./reordercat61 files/text20meg.txt > files/out.txt",
    "regular large file, 4KB block I/O, random seek order");

run(18,
    "./reordercat61 -r 6582 files/text20meg.txt > files/out.txt",
    "regular large file, 4KB block I/O, random seek order");

run(19,
    "./stridecat61 -t 1048576 files/text5meg.txt > files/out.txt",
    "regular medium file, character I/O, 1MB stride order");

run(20,
    "./stridecat61 -t 2 files/text5meg.txt > files/out.txt",
    "regular medium file, character I/O, 2B stride order");


# PIPE FILES, SEQUENTIAL I/O

run(21,
    "cat files/text1meg.txt | ./cat61 | cat > files/out.txt",
    "piped small file, character I/O, sequential");

run(22,
    "cat files/text20meg.txt | ./cat61 | cat > files/out.txt",
    "piped large file, character I/O, sequential");

run(23,
    "cat files/text5meg.txt | ./blockcat61 -b 1024 | cat > files/out.txt",
    "piped medium file, 1KB block I/O, sequential");

run(24,
    "cat files/text20meg.txt | ./blockcat61 | cat > files/out.txt",
    "piped large file, 4KB block I/O, sequential");

run(25,
    "cat files/text20meg.txt | ./randblockcat61 | cat > files/out.txt",
    "piped large file, 1B-4KB block I/O, sequential");


summary();

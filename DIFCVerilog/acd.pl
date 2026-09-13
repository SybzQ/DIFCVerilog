#!/usr/bin/perl
use strict;
use warnings;
use Term::ANSIColor;

# Subroutines definition
sub print_ok { print colored("verified\n", 'green') }
sub print_fail { print colored("fail\n", 'red') }

my $assert; # Declare $assert here
my $file = shift @ARGV or die "Usage: $0 <input_file>\n";

# NOTE: This script invokes the Z3 SMT solver as an external backend.
# The variable names $z3home, $z3option, and $z3bin reflect this dependency.
my $z3home = ""; # specify z3 home directory
my $z3option = "smt2";

my $fail_counter = 0;
my $counter = 0;
my @assertions = ();

my $z3bin = "$z3home"."z3";
my $str = `$z3bin -$z3option $file`;

open(FILE, '<', $file) or die "Can't read file $file\n";

while (<FILE>) {
    my $assertion; # Redeclare $assertion in this scope
    my $isassertion; # Redeclare $isassertion in this scope

    if (m/^\(push\)/) {
        $assertion = "";
        $isassertion = 1;
    } elsif (m/^\(check-sat\)/) {
        push(@assertions, $assertion);
        $isassertion = 0;
    } elsif ($isassertion) {
        $assertion = $_;
    }
}

close(FILE);

my $errors = "";
for(split /^/, $str) {
    if (/^sat/) {
        $assert = $assertions[$counter];
        $errors .= $assert;
        $fail_counter ++;
        $counter ++;
    } elsif (/^unsat/) {
        $counter ++;
    }
}

if ($errors eq "") {
    print_ok();
} else {
    print_fail();
    print $errors;
}


print "Total: $fail_counter assertions failed\n";
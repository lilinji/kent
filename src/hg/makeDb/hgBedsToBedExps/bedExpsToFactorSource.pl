#!/usr/bin/env perl

# bedExpsToFactorSource.pl - create clusters file in BED5+ format, with additional fields:
#       6: expCount: number of experiments having non-zero scores
#       7: expIds: IDs of experiments
#       8: expScores: scores
#
# Created to provide a more efficient representation for UCSC factorSource type tracks.
# The input clusters file is UCSC BED15 format (see genome.ucsc.edu/FAQ/FAQformat.html);
# This format is used in later versions of ENCODE TFBS Clusters tracks.

use warnings;
use strict;

scalar(@ARGV) == 1 or die "usage: bedExpsToFactorSource.pl clusters.bed\n";

my $clusterFile = $ARGV[0];

# read clusters file in BED5 format (generated by hgBedExpsToBed)
open(CF, $clusterFile) or die "can't read $clusterFile\n";
while (<CF>) {
    chomp;
    my ($chrom, $start, $end, $name, $score) = split('\t');
    my @fields = split("\t");
    my @expIds = split(",", $fields[13]);
    my @expScores = split(",", $fields[14]);
    my @exps;
    my @scores;
    my $i = -1;
    my $expCount = 0;

    # grab experiments with non-zero scores and look up source
    foreach my $expScore (@expScores) {
        $i++;
        next if (!$expScore);
        $expCount++;
        my $exp = $expIds[$i];
        push(@exps, $exp);
        push(@scores, $expScore);
    }
    my $exps = join(',', @exps);
    my $scores = join(',', @scores);

    # output BED 5+ line
    print join("\t", $chrom, $start, $end, $name, $score, $expCount, $exps, $scores), "\n";
}
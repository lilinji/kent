# stsMapRat.sql was originally generated by the autoSql program, which also 
# generated stsMapRat.c and stsMapRat.h.  This creates the database representation of
# an object which can be loaded and saved from RAM in a fairly 
# automatic way.

#STS marker and its position on golden path and various maps
CREATE TABLE stsMapRat (
    bin smallint unsigned not null,              # speed index
    chrom varchar(255) not null,	# Chromosome or 'unknown'
    chromStart int not null,	# Start position in chrom - negative 1 if unpositioned
    chromEnd int unsigned not null,	# End position in chrom
    name varchar(255) not null,	# Name of STS marker
    score int unsigned not null,	# Score of a marker = 1000 when placed uniquely, else 1500/(#placements) when placed in multiple locations
    identNo int unsigned not null,	# Identification number of STS
    ctgAcc varchar(255) not null,	# Contig accession number
    otherAcc varchar(255) not null,	# Accession number of other contigs that the marker hits
    rhChrom varchar(255) not null,	# Chromosome (no chr) from RH map or 0 if none
    rhPos float not null,	# Position on rh map
    rhLod float not null,	# Lod score of RH map
    fhhChr varchar(255) not null,	# Chromosome (no chr) from FHHxACI genetic or 0 if none
    fhhPos float not null,	# Position on FHHxACI map
    shrspChrom varchar(255) not null,	# Chromosome (no chr) from SHRSPxBN geneticmap or 0 if none
    shrspPos float not null,	# Position on SHRSPxBN genetic map
              #Indices
    INDEX(identNo),
    INDEX(chrom(8),chromStart)

);

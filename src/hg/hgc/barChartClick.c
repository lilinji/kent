/* Details pages for barChart tracks */

/* Copyright (C) 2015 The Regents of the University of California 
 * See README in this or parent directory for licensing information. */

#include "common.h"
#include "hash.h"
#include "hdb.h"
#include "hvGfx.h"
#include "trashDir.h"
#include "hgc.h"
#include "hCommon.h"

#include "barChartBed.h"
#include "barChartCategory.h"
#include "barChartData.h"
#include "barChartSample.h"
#include "barChartUi.h"

struct barChartItemData
/* Measured value for a sample and the sample category at a locus.
 * Used for barChart track details (boxplot) */
    {
    struct barChartItemData *next;  /* Next in singly linked list. */
    char *sample;	/* Sample identifier */
    char *category;     /* Sample category (from barChartSample table  or barChartSampleUrl file) */
    double value;	/* Measured value (e.g. expression level) */
    };

static struct hash *getTrackCategories(struct trackDb *tdb)
/* Get list of categories from trackDb.  This may be a subset of those in matrix. 
 * (though maybe better to prune matrix for performance) */
{
char *categs = trackDbSetting(tdb, BAR_CHART_CATEGORY_LABELS);
char *words[BAR_CHART_MAX_CATEGORIES];
int wordCt;
wordCt = chopLine(cloneString(categs), words);
int i;
struct hash *categoryHash = hashNew(0);
for (i=0; i<wordCt; i++)
    {
    hashStore(categoryHash, words[i]);
    }
return categoryHash;
}

static struct barChartBed *getBarChartFromFile(char *item, char *chrom, int start, int end, 
                                                char *file)
/* Retrieve barChart BED item from big file */
{
struct bbiFile *bbi = bigBedFileOpen(file);
struct lm *lm = lmInit(0);
struct bigBedInterval *bb, *bbList =  bigBedIntervalQuery(bbi, chrom, start, end, 0, lm);
for (bb = bbList; bb != NULL; bb = bb->next)
    {
    char startBuf[16], endBuf[16];
    char *row[32];
    bigBedIntervalToRow(bb, chrom, startBuf, endBuf, row, ArraySize(row));
    struct barChartBed *barChart = barChartBedLoadOptionalOffsets(row, TRUE);
    if (sameString(barChart->name, item))
        return barChart;
    }
return NULL;
}

static struct barChartBed *getBarChartFromTable(char *item, char *chrom, int start, int end, 
                                                char *table)
/* Retrieve barChart BED item from track table */
{
struct barChartBed *barChart = NULL;
struct sqlConnection *conn = hAllocConn(database);
char **row;
char query[512];
struct sqlResult *sr;
if (sqlTableExists(conn, table))
    {
    sqlSafef(query, sizeof query, 
                "SELECT * FROM %s WHERE name='%s'"
                    "AND chrom='%s' AND chromStart=%d AND chromEnd=%d", 
                                table, item, chrom, start, end);
    sr = sqlGetResult(conn, query);
    row = sqlNextRow(sr);
    // TODO: Fix or retire
    /*
    boolean hasOffsets = sqlColumnExists(conn, table, BARCHART_OFFSET_COLUMN);
    */

    if (row != NULL)
        {
        // TODO: Fix or retire
        barChart = barChartBedLoadOptionalOffsets(row, FALSE);
        }
    sqlFreeResult(&sr);
    }
hFreeConn(&conn);
return barChart;
}

static struct barChartBed *getBarChart(char *item, char *chrom, int start, int end, 
                                        struct trackDb *tdb)
/* Retrieve barChart BED item from track */
{
struct barChartBed *barChart = NULL;
char *file = trackDbSetting(tdb, "bigDataUrl");
if (file != NULL)
    barChart = getBarChartFromFile(item, chrom, start, end, file);
else
    barChart = getBarChartFromTable(item, chrom, start, end, tdb->table);
return barChart;
}

static struct barChartItemData *getSampleValsFromFile(struct trackDb *tdb, 
                                                        struct hash *categoryHash,
                                                        struct barChartBed *bed)
/* Get all data values in a file for this item (locus) */
{
// Get sample categories from sample file
// Format: id, category, extras
char *url = trackDbSetting(tdb, "barChartSampleUrl");
struct lineFile *lf = udcWrapShortLineFile(url, NULL, 0);
struct hash *sampleHash = hashNew(0);
char *words[2];
int sampleCt = 0;
while (lineFileChopNext(lf, words, sizeof words))
    {
    hashAdd(sampleHash, words[0], words[1]);
    sampleCt++;
    }
lineFileClose(&lf);

// Open matrix file
url = trackDbSetting(tdb, "barChartDataUrl");
struct udcFile *f = udcFileOpen(url, NULL);

// Get header line with sample ids
char *header = udcReadLine(f);
int wordCt = sampleCt+1;        // initial field is label or empty 
char **samples;
AllocArray(samples, wordCt);
chopByWhite(header, samples, wordCt);

// Get data values
// Format: id, category, extras
// TODO: check data types for offset & len
bits64 offset = (bits64)bed->_dataOffset;
bits64 size = (bits64)bed->_dataLen;
udcSeek(f, offset);
bits64 seek = udcTell(f);
if (udcTell(f) != offset)
    warn("UDC seek mismatch: expecting %Lx, got %Lx. ", offset, seek);
char *buf = needMem(size);
bits64 count = udcRead(f, buf, size);
if (count != size)
    warn("UDC read mismatch: expecting %Ld bytes, got %Ld. ", size, count);
char **vals;
AllocArray(vals, wordCt);
chopByWhite(buf, vals, wordCt);
udcFileClose(&f);

// Construct list of sample data with category
struct barChartItemData *sampleVals = NULL, *data = NULL;
int i;
for (i=1; i<wordCt; i++)
    {
    char *sample = samples[i];
    char *categ = (char *)hashFindVal(sampleHash, sample);
    if (categ == NULL)
        warn("barChart track %s: unknown category for sample %s", tdb->track, sample);
    else if (hashLookup(categoryHash, categ))
        {
        AllocVar(data);
        // TODO: try w/o clone
        data->sample = cloneString(sample);
        data->category = cloneString(categ);
        data->value = sqlDouble(vals[i]);
        slAddHead(&sampleVals, data);
        }
    }
return sampleVals;
}

static struct sqlConnection *getConnectionAndTable(struct trackDb *tdb, char *suffix,
                                                         char **retTable)
/* Look for <table><suffix> in database or hgFixed and set up connection */
{
char table[256];
assert(retTable);
safef(table, sizeof(table), "%s%s", tdb->table, suffix);
*retTable = cloneString(table);
struct sqlConnection *conn = hAllocConn(database);
if (!sqlTableExists(conn, table))
    {
    hFreeConn(&conn);
    conn = hAllocConn("hgFixed");
    if (!sqlTableExists(conn, table))
        {
        hFreeConn(&conn);
        return NULL;
        }
    }
return conn;
}

static struct barChartItemData *getSampleValsFromTable(struct trackDb *tdb, 
                                                        struct hash *categoryHash,
                                                        struct barChartBed *bed)
/* Get data values for this item (locus) from all samples */
// TODO: Consider retiring table-based version.  Use files instead, like hub version
{
char *table = NULL;
struct sqlConnection *conn = getConnectionAndTable(tdb, "Data", &table);
struct barChartData *val, *vals = barChartDataLoadForLocus(conn, table, bed->name);
hFreeConn(&conn);

// Get category for samples 
conn = getConnectionAndTable(tdb, "Sample", &table);
char query[512];
sqlSafef(query, sizeof(query), "SELECT * FROM %s", table);
struct barChartSample *sample, *samples = barChartSampleLoadByQuery(conn, query);
hFreeConn(&conn);

struct hash *sampleHash = hashNew(0);
for (sample = samples; sample != NULL; sample = sample->next)
    {
    hashAdd(sampleHash, sample->sample, sample);
    }

// Construct list of sample data with category
struct barChartItemData *sampleVals = NULL, *data = NULL;
for (val = vals; val != NULL; val = val->next)
    {
    struct barChartSample *sample = hashFindVal(sampleHash, val->sample);
    if (sample == NULL)
        warn("barChart track %s: unknown category for sample %s", tdb->track, val->sample);
    else if (hashLookup(categoryHash, sample->category))
        {
        AllocVar(data);
        data->sample = cloneString(val->sample);
        data->category = cloneString(sample->category);
        data->value = val->value;
        slAddHead(&sampleVals, data);
        }
    }
return sampleVals;
}

static struct barChartItemData *getSampleVals(struct trackDb *tdb, struct barChartBed *chartItem)
/* Get data values for this item (locus) from all samples */
{
struct barChartItemData *vals = NULL;
char *file = trackDbSetting(tdb, "barChartDataUrl");
struct hash *categoryHash = getTrackCategories(tdb);
if (file != NULL)
    vals = getSampleValsFromFile(tdb, categoryHash, chartItem);
else
    vals = getSampleValsFromTable(tdb, categoryHash, chartItem);
return vals;
}

static char *makeDataFrame(char *track, struct barChartItemData *vals)
/* Create R data frame from sample data.  This is a tab-sep file, one row per sample.
   Return filename. */
{

// Create data frame with columns for sample, category, value */
struct tempName dfTn;
trashDirFile(&dfTn, "hgc", "barChart", ".df.txt");
FILE *f = fopen(dfTn.forCgi, "w");
if (f == NULL)
    errAbort("can't create temp file %s", dfTn.forCgi);
fprintf(f, "sample\tcategory\tvalue\n");
int i = 0;
struct barChartItemData *val;
for (val = vals; val != NULL; val = val->next)
    {
    // NOTE: don't actually need sample ID here -- just use a unique int
    fprintf(f, "%d\t%s\t%0.3f\n", i++, val->category, val->value);
    }
fclose(f);
return cloneString(dfTn.forCgi);
}

char *makeColorFile(struct trackDb *tdb)
/* Make a file with category + color */
{
struct tempName colorTn;
trashDirFile(&colorTn, "hgc", "barChartColors", ".txt");
FILE *f = fopen(colorTn.forCgi, "w");
if (f == NULL)
    errAbort("can't create temp file %s", colorTn.forCgi);
struct barChartCategory *categs = barChartUiGetCategories(database, tdb);
struct barChartCategory *categ;
fprintf(f, "%s\t%s\n", "category", "color");
for (categ = categs; categ != NULL; categ = categ->next)
    {
    //fprintf(f, "%s\t#%06X\n", categ->label, categ->color);
    fprintf(f, "%s\t%d\n", categ->label, categ->color);
    }
fclose(f);
return cloneString(colorTn.forCgi);
}

static void printBoxplot(char *df, char *item, char *units, char *colorFile)
/* Plot data frame to image file and include in HTML */
{
struct tempName pngTn;
trashDirFile(&pngTn, "hgc", "barChart", ".png");

/* Exec R in quiet mode, without reading/saving environment or workspace */
char cmd[256];
safef(cmd, sizeof(cmd), "Rscript --vanilla --slave hgcData/barChartBoxplot.R %s %s %s %s %s",
                                item, units, colorFile, df, pngTn.forHtml);
int ret = system(cmd);
if (ret == 0)
    printf("<img src = \"%s\" border=1><br>\n", pngTn.forHtml);
}

void doBarChartDetails(struct trackDb *tdb, char *item)
/* Details of barChart item */
{
int start = cartInt(cart, "o");
int end = cartInt(cart, "t");
struct barChartBed *chartItem = getBarChart(item, seqName, start, end, tdb);
if (chartItem == NULL)
    errAbort("Can't find item %s in barChart table/file %s\n", item, tdb->table);

genericHeader(tdb, item);
int categId;
float highLevel = barChartMaxValue(chartItem, &categId);
char *units = trackDbSettingClosestToHomeOrDefault(tdb, BAR_CHART_UNIT, "");
printf("<b>Maximum value: </b> %0.2f %s in %s<br>\n", 
                highLevel, units, barChartUiGetCategoryLabelById(categId, database, tdb));
printf("<b>Total all values: </b> %0.2f<br>\n", barChartTotalValue(chartItem));
printf("<b>Score: </b> %d<br>\n", chartItem->score); 
printf("<b>Genomic position: "
                "</b>%s <a href='%s&db=%s&position=%s%%3A%d-%d'>%s:%d-%d</a><br>\n", 
                    database, hgTracksPathAndSettings(), database, 
                    chartItem->chrom, chartItem->chromStart+1, chartItem->chromEnd,
                    chartItem->chrom, chartItem->chromStart+1, chartItem->chromEnd);
struct barChartItemData *vals = getSampleVals(tdb, chartItem);
if (vals != NULL)
    {
    // Print boxplot
    puts("<p>");
    char *df = makeDataFrame(tdb->table, vals);
    char *colorFile = makeColorFile(tdb);
    printBoxplot(df, item, units, colorFile);
    }
puts("<br>");
printTrackHtml(tdb);
}

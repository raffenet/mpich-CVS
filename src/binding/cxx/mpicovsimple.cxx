//-*- Mode: C++; c-basic-offset:4 ; -*- */
//
//  (C) 2004 by Argonne National Laboratory.
//      See COPYRIGHT in top-level directory.
//
// This is a *very* simple tool for basic coverage analysis.  
// This is intended as a stop-gap until gcov works with the C++ files
// used in the MPICH2 binding of C++ (as of 2/23/2004, gcov aborts when
// processing the coverage files produced by g++ for the MPICH2 C++
// binding).
//

#include <iostream.h>
#include <fstream.h>

#include "mpicovsimple.h"

// Find the matching entry or insert and initialize a new one
covinfo * MPIX_Coverage::findOrInsert( const char name[], int argcount )
{
    covinfo *p = head, *newp;
    int cmp;
    static covinfo *lastp = 0;    // We use lastp to start where we
                                  // left off if we can; this speeds
                                  // sorted inserts
    
    // See if we can skip ahead...
    if (lastp) {
	cmp = strcmp( lastp->name, name );
	if (cmp < 0 || (cmp == 0 && lastp->argcount <= argcount )) p = lastp;
    }

    while (p) {
	cmp = strcmp( p->name, name );
	if (cmp == 0) {
	    cmp = p->argcount - argcount;
	    if (cmp == 0) { 
		// If still 0, we've found our match.
		lastp = p;
		return p;
	    }
	}

	if (cmp > 0) {
	    // If we got here, backup and exit to perform the insert
	    p = p->bLink;
	    break;
	}
	// If we're at the end of the list, this p is the last element,
	// so we exit now
	if (!p->fLink) break;

	p = p->fLink;
    }
    
    // If we got here, we need to insert after p
    newp             = new covinfo;
    newp->name       = new char [strlen(name)+1];
    strcpy( newp->name, name );
    newp->argcount   = argcount;
    newp->count      = 0;
    newp->sourceFile = 0;
    newp->firstLine  = -1;
    newp->lastLine   = -1;
    newp->bLink      = p;
    if (p) {
	// insert after p
	newp->fLink = p->fLink;
	p->fLink    = newp;
	if (newp->fLink) {
	    newp->fLink->bLink = newp;
	}
    }
    else {
	// insert at the head of the list
	newp->fLink = head;
	if (head) { 
	    head->bLink = newp; 
	}
	head        = newp;
    }

    lastp = newp;
    return newp;
}

// Add an item to the coverage list, including the first line
void MPIX_Coverage::Add( const char name[], int argcnt, 
		    const char file[], int line )
{
    covinfo *p = findOrInsert( name, argcnt );

    p->count ++;
    if (!p->sourceFile) {
	p->sourceFile = new char [strlen(file)+1];
	strcpy( p->sourceFile, file );
	p->firstLine = line;
    }
}

// Add the last line value to an item in the coverage list
void MPIX_Coverage::AddEnd( const char name[], int argcnt,
		       const char file[], int line )
{
    covinfo *p = findOrInsert( name, argcnt );

    if (p->lastLine < 0) 
	p->lastLine = line;
}

//
// Merge the coverage data with the data in the file
// The directory for the coverage file is defined by the
// cpp value COVERAGE_DIR
int MPIX_Coverage::FileMerge( const char filename[] )
{
    ifstream infile;
    ofstream outfile;
    covinfo *p;
    covinfo fp;
    char *infilename, *tmpfilename;

    // Try to open the input file.  
    {
#ifdef COVERAGE_DIR
	infilename = new char [strlen(filename) + strlen(COVERAGE_DIR) + 2 ];
	strcpy( infilename, COVERAGE_DIR );
	strcat( infilename, "/" );
	strcat( infilename, filename );
#else
	infilename = (char *)filename;
#endif
	infile.open( infilename );

	// Create a place in which to read the data
	fp.name       = new char [1024];
	fp.sourceFile = new char [1024];

	// Add the contents of the file to the internal list
	// This is easy but not the most memory efficient.
	// If this becomes a problem, we can merge the two
	// into a single output
	while (infile) {
	    fp.count = -1;  // Set a sentinal on eof in infile
	    infile >> fp.name >> fp.argcount >> fp.count >> fp.sourceFile >>
		fp.firstLine >> fp.lastLine;
	    if (fp.count == -1) break;
	    p = findOrInsert( fp.name, fp.argcount );
	    if (!p->sourceFile) {
		p->sourceFile = strdup( fp.sourceFile );
		p->firstLine  = fp.firstLine;
		p->lastLine   = fp.lastLine;
	    }
	    p->count += fp.count;
	}
	infile.close();
	// Recover new storage
	delete fp.name;
	delete fp.sourceFile;
    }
    
    // Try to open the output file
#ifdef COVERAGE_DIR
    tmpfilename = new char [strlen(".covtmp") + strlen(COVERAGE_DIR) + 2 ];
    strcpy( tmpfilename, COVERAGE_DIR );
    strcat( tmpfilename, "/" );
    strcat( tmpfilename, ".covtmp" );
#else
    tmpfilename = ".covtmp";
#endif
    outfile.open( tmpfilename );

    p = head;
    while (p) {
	outfile << p->name << '\t' << p->argcount << '\t' << 
	    p->count << '\t' << p->sourceFile << '\t' <<
	    p->firstLine << '\t' << p->lastLine << '\n';
	p = p->fLink;
    }
    
    outfile.close();
    
    // Now, remove the old file and move the new file over it
    remove( infilename );
    rename( tmpfilename, infilename );

    return 0;
}

MPIX_Coverage MPIR_Cov;

//#define TEST_PROGRAM
#ifdef TEST_PROGRAM
int main( int argc, char **argv )
{
    MPIR_Cov.Add( "foo", 2, __FILE__, __LINE__ );
    MPIR_Cov.Add( "foo", 1, __FILE__, __LINE__ );
    MPIR_Cov.Add( "bar", 2, __FILE__, __LINE__ );
    
    MPIR_Cov.FileMerge( "covtest.dat" );
    return 0;
}
#endif

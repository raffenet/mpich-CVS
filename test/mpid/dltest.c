/* */
int counter = 1;

int init(void) {
    int a = counter;
    counter++;
    a = upcall( a );
    if (a != counter) {
	counter --;
    }
}
int finalize( int offset )
{
    int rc = 1;
    if (counter != offset) {
	rc = 0;
    }
    return rc;
}

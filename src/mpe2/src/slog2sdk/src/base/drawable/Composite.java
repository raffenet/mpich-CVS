/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.drawable;

import java.awt.Graphics2D;
import java.awt.Point;
import java.util.Map;
import java.util.SortedSet;
import java.util.TreeSet;
import java.util.Iterator;
import java.util.Arrays;
import java.io.ByteArrayInputStream;

import base.io.MixedDataInputStream;
import base.io.MixedDataInput;
import base.io.MixedDataOutput;

// Composite should be considered as an InfoBox of Primitive[]

// Cloneable interface is for the creation of Shadow. 
public class Composite extends Drawable
//                       implements Cloneable
{
    private static final int INIT_BYTESIZE   = 2  /* primes.length */ ; 

    private static final DrawOrderComparator DRAWING_ORDER
                                             = new DrawOrderComparator();

    private   Primitive[]      primes;
    private   int              last_prime_idx;

    public Composite()
    {
        super();
        primes     = null;
    }

    public Composite( int Nprimes )
    {
        super();
        primes          = new Primitive[ Nprimes ];
        last_prime_idx  = primes.length - 1;
    }

    public Composite( Category in_type, int Nprimes )
    {
        super( in_type );
        primes          = new Primitive[ Nprimes ];
        last_prime_idx  = primes.length - 1;
    }

    //  This is NOT a copy constructor,
    //  only Category and InfoType[] are copied, not InfoValue[].
/*
    public Composite( final Composite cmplx )
    {
        super( cmplx );
        Primitive[] cmplx_primes = cmplx.primes;
        primes        = new Primitive[ cmplx_primes.length ];
        for ( int idx = 0; idx < primes.length; idx++ )
            primes[ idx ] = new Primitive( cmplx_primes[ idx ] );
        last_prime_idx    = primes.length - 1;
    }

    public Composite( Category in_type, final Composite cmplx )
    {
        super( in_type, cmplx );
        Primitive[] cmplx_primes = cmplx.primes;
        primes        = new Primitive[ cmplx_primes.length ];
        for ( int idx = 0; idx < primes.length; idx++ )
            primes[ idx ] = new Primitive( cmplx_primes[ idx ] );
        last_prime_idx    = primes.length - 1;
    }
*/

    //  For the support of JNI used in TRACE-API
    public Composite( int type_idx, double starttime, double endtime,
                      Primitive[] in_primes, byte[] byte_infovals )
    {
        super( type_idx, byte_infovals );
        super.setEarliestTime( starttime );
        super.setLatestTime( endtime );
        primes = in_primes;
    }

    //  For SLOG-2 input API
    public boolean resolveCategory( final Map categorymap )
    {
        Primitive prime;
        boolean  allOK, isOK;
        allOK = super.resolveCategory( categorymap );
        // InfoBox.resolveCategory( Map )
        if ( primes != null )
            for ( int idx = primes.length-1; idx >= 0; idx-- ) {
                 prime = primes[ idx ];
                 if ( prime != null ) {
                     isOK  = prime.resolveCategory( categorymap );
                     allOK = allOK && isOK;
                 }
            }
        return allOK;
    }   

/*
    public void setInfoValues()
    {
        Primitive prime;

        super.setInfoValues();
        if ( primes != null )
            for ( int idx = primes.length-1; idx >= 0; idx-- ) {
                 prime = primes[ idx ];
                 if ( prime != null )
                     prime.setInfoValues();
            }
    }
*/

    public int getNumOfPrimitives()
    {
        if ( primes != null )
            return primes.length;
        else
            return 0;
    }

    public int getByteSize()
    {
        int bytesize;
        //  Match the disk storage strategy in
        //  slog2.output.TreeNode.add( Drawable )
        //  if ( Category in Composite )
        //     => save the whole Composite,
        //     => will invoke Composite.writeObject()
        //  if ( No Category in Composite )
        //     => save the individual Primitive[],
        //     => will NOT invoke Composite.writeObject()
        if ( super.getCategory() != null ) 
            bytesize = super.getByteSize() + INIT_BYTESIZE;
        else
            bytesize = 0;

        if ( primes != null )
            for ( int idx = primes.length-1; idx >= 0; idx-- )
                bytesize += primes[ idx ].getByteSize();
        return bytesize;
    }

    public void writeObject( MixedDataOutput outs )
    throws java.io.IOException
    {
        int primes_length, idx;

        // Save the Lists in Increasing Starttime order
        Arrays.sort( primes, DRAWING_ORDER );

        super.writeObject( outs );

        primes_length = (short) primes.length;
        outs.writeShort( primes_length );
        for ( idx = 0; idx < primes_length; idx++ )
            primes[ idx ].writeObject( outs );
    }

    public Composite( MixedDataInput ins )
    throws java.io.IOException
    {
        super();     // InfoBox();
        this.readObject( ins );
    }

    public void readObject( MixedDataInput ins )
    throws java.io.IOException
    {
        Primitive  prime;
        short     idx, Nprimes;

        super.readObject( ins );

        Nprimes  = ins.readShort();
        primes   = new Primitive[ Nprimes ];
        for ( idx = 0; idx < primes.length; idx++ ) {
            prime         = new Primitive( ins );
            primes[ idx ] = prime;
            // Determine the SuperClass, TimeBoundingBox.
            super.affectTimeBounds( prime );
        }
        last_prime_idx  = primes.length - 1;
    }

    /*
        Primitives related operation:
    */

    // 0 <= prime_order < primes.length
    public void setPrimitive( int prime_idx, final Primitive prime )
    throws ArrayIndexOutOfBoundsException
    {
        if ( prime_idx < 0 || prime_idx >= primes.length ) {
            throw new ArrayIndexOutOfBoundsException( "input index, "
                                                    + prime_idx
                                                    + ", is out of range, [0.."
                                                    + primes.length + "]." );
        }
        primes[ prime_idx ] = prime;
        super.affectTimeBounds( prime );
    }

    public Primitive getPrimitive( int prime_idx )
    throws ArrayIndexOutOfBoundsException
    {
        if ( prime_idx < 0 || prime_idx >= primes.length ) {
            throw new ArrayIndexOutOfBoundsException( "input index, "
                                                    + prime_idx
                                                    + ", is out of range, [0.."
                                                    + primes.length + "]." );
        }
        return primes[ prime_idx ];
    }

    public void setPrimitives( final Primitive[] in_primes )
    throws IllegalArgumentException
    {
        if ( in_primes.length != primes.length ) {
            throw new IllegalArgumentException( "input array size, "
                                              + in_primes.length + ", is "
                                              + "different from the original, "
                                              + primes.length );
        }
        primes = in_primes;
        for ( int idx = primes.length-1; idx >= 0; idx-- )
            super.affectTimeBounds( primes[ idx ] );
    }

    public Primitive[] getPrimitives()
    {
        return primes;
    }

    public Iterator timeLapIterator( final TimeBoundingBox  tframe )
    {
        return new ItrOfPrimes( tframe );
    }

    //  API to support Shadow generation ??????
    public SortedSet getSetOfPrimitiveLineIDs()
    {
        SortedSet lineIDs = new TreeSet();
        for ( int idx = 0; idx < primes.length; idx++ )
            lineIDs.addAll( primes[ idx ].getListOfVertexLineIDs() );
        return lineIDs;
    }

    //  Used to generate IdentityLineIDMap  ?????? 
    public Integer[] getArrayOfLineIDs()
    {
        SortedSet lineIDset = this.getSetOfPrimitiveLineIDs();
        Integer[] lineIDary = new Integer[ lineIDset.size() ];
        lineIDset.toArray( lineIDary );
        return lineIDary;
    }

    /*
        setStartPrimitive()/setFinalPrimitive()
       /getStartPrimitive()/getFinalPrimitive()
       are stilll good for Composite with only ONE primitive.
    */
    public void setStartPrimitive( final Primitive start_prime )
    {
        primes[ 0 ] = start_prime;
        super.affectTimeBounds( start_prime );
    }

    public void setFinalPrimitive( final Primitive final_prime )
    {
        primes[ last_prime_idx ] = final_prime;
        super.affectTimeBounds( final_prime );
    }

    public Primitive getStartPrimitive()
    {
        return primes[ 0 ];
    }

    public Primitive getFinalPrimitive()
    {
        return primes[ last_prime_idx ];
    }

    public String toString()
    {
        StringBuffer rep;
        int idx;

        rep = new StringBuffer( "Composite[ " + super.toString() + " " );
        for ( idx = 0; idx < primes.length; idx++ )
            rep.append( primes[ idx ].toString() + " " );
        rep.append( "]" );
        return rep.toString();
    }

    private class ItrOfPrimes implements Iterator
    {
        private TimeBoundingBox  timeframe;
        private Primitive        next_primitive;
        private int              next_prime_idx;

        public ItrOfPrimes( final TimeBoundingBox  tframe )
        {
            timeframe       = tframe;
            next_primitive  = null;
            next_prime_idx  = 0;
        }

        public boolean hasNext()
        {
            if ( primes == null )
                return false;

            while ( next_prime_idx < primes.length ) {
                if ( primes[ next_prime_idx ].overlaps( timeframe ) ) {
                    next_primitive  = primes[ next_prime_idx ];
                    next_prime_idx++;
                    return true;
                }
            }
            return false;
        }

        public Object next()
        {
            return next_primitive;
        }

        public void remove() {}
    }   // private class ItrOfPrimes 

/*
    //  Check clone() interface
    public static final void main( String[] args )
    {
        Composite prime, sobj;

        Category ctgy = new Category();  // incomplete category
        ctgy.setInfoKeys( "msg_tag\nmsg_size\n" );
        prime = new Composite( ctgy, new Primitive[] { new Primitive( 1.1, 1 ),
                                                 new Primitive( 2.2, 2 ) } );
        prime.setInfoValue( 0, 10 );
        prime.setInfoValue( 1, 1024 );
        System.out.println( "prime = " + prime );

        sobj = null;
        try {
            sobj = (Composite) prime.clone();
        } catch( CloneNotSupportedException cerr ) {
            cerr.printStackTrace();
            System.exit( 1 );
        }

        System.out.println( "\nAfter cloning" );
        System.out.println( "prime = " + prime );
        System.out.println( "sobj = " + sobj );

        sobj.getStartPrimitive().time = 4.4;
        sobj.getFinalPrimitive().time = 5.5;
        sobj.setInfoValue( 0, 1000 );
        System.out.println( "\nAfter modification of the clone" );
        System.out.println( "prime = " + prime );
        System.out.println( "sobj = " + sobj );
        System.out.println( "This proves that clone() is useless for Shadow" );
    }
*/

    public boolean isTimeOrdered()
    {
        Primitive  prime;
        int        primes_length, idx;
        if ( ! super.isTimeOrdered() ) {
            System.err.println( "**** Violation of Causality ****\n"
                              + "Offending Composite -> " + this );
            return false;
        }
        primes_length = (short) primes.length;
        for ( idx = 0; idx < primes_length; idx++ ) {
            prime = primes[ idx ];
            if ( ! prime.isTimeOrdered() ) {
                System.err.println( "**** Internal Primitive Error ****\n"
                                  + "It is number " + idx + " primitive "
                                  + "in the composite." );
                return false;
            }
            if ( ! super.covers( prime ) ) {
                System.err.println( "**** Out of Composite Time Range ****\n"
                                  + "Offending Primitive -> " + this + "\n"
                                  + "\t time coordinate " + idx
                                  + " is out of range." );
                return false;
            }
        }
        return true;
    }

     /* Caller needs to be sure that the Drawable is a State */
    public void setStateRowAndNesting( CoordPixelXform  coord_xform,
                                       Map              map_line2row,
                                       NestingStacks    nesting_stacks )
    {
        Primitive  prime;
        int        primes_length, idx;
        primes_length = (short) primes.length;
        // primes[] needs to be iterated in increaing starttime order
        for ( idx = 0; idx < primes_length; idx++ ) {
            prime = primes[ idx ];
            // assume all primitives are all States
            if ( coord_xform.overlaps( prime ) )
                prime.setStateRowAndNesting( coord_xform, map_line2row,
                                             nesting_stacks );
        }
    }

    public int  drawOnCanvas( Graphics2D g, CoordPixelXform coord_xform,
                              Map map_line2row, DrawnBoxSet drawn_boxes )
    {
        Primitive  prime;
        int        primes_length, num_primes_drawn, idx;

        num_primes_drawn = 0;
        primes_length = (short) primes.length;
        // primes[] needs to be iterated in increaing starttime order
        for ( idx = 0; idx < primes_length; idx++ ) {
            prime = primes[ idx ];
            if ( coord_xform.overlaps( prime ) )
                num_primes_drawn += prime.drawOnCanvas( g, coord_xform,
                                                        map_line2row,
                                                        drawn_boxes );
        }
        return num_primes_drawn;
    }

    public Drawable getDrawableWithPixel( CoordPixelXform coord_xform,
                                          Map             map_line2row,
                                          Point           pix_pt )
    {
        Primitive  prime;
        int        primes_length, idx;

        primes_length = (short) primes.length;
        for ( idx = primes_length-1; idx >= 0; idx-- ) {
            prime = primes[ idx ];
            if ( coord_xform.overlaps( prime ) ) {
                if ( prime.getDrawableWithPixel( coord_xform,
                                                 map_line2row, pix_pt )
                     != null )
                    return prime;
            }
        }
        return null;
    }

    public boolean containSearchable()
    {
        int        primes_length, idx;

        primes_length = (short) primes.length;
        for ( idx = 0; idx < primes_length; idx++ ) {
            if ( primes[ idx ].containSearchable() )
                return true;
        }
        return false;
    }
}

/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.slog2;

import java.util.Map;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.ListIterator;
import java.util.Collections;

import base.io.MixedDataInput;
import base.io.MixedDataOutput;
import base.drawable.DrawOrderComparator;
import base.drawable.TimeBoundingBox;
import base.drawable.Drawable;
import base.drawable.Primitive;
import base.drawable.Composite;

public class BufForDrawables extends BufForObjects
{
    private static final int  INIT_BYTESIZE = BufForObjects.BYTESIZE
                                            + 4  /* buf4nestable.size() */
                                            + 4  /* buf4nestless.size() */ ;
    private static final DrawOrderComparator DRAWING_ORDER
                                             = new DrawOrderComparator();

    private static final byte PRIMITIVE_ID  = 0;
    private static final byte COMPOSITE_ID  = 1;

    /*
        nestable = drawables that can be nested nicely, like state-like object
        nestless = drawables that need no nesting order, e.g. arrow/event
    */
    private List              buf4nestable;   /* state and composite */
    private List              buf4nestless;   /* arrow/event */

    /*  
        isOutputBuf = true  when BufForDrawables is used in Output API
        isOutputBuf = false when BufForDrawables is used in Input API
    */
    private boolean           isOutputBuf;
    private boolean           haveObjectsBeenSaved;
    private int               total_bytesize; // bytesize for the disk footprint

    public BufForDrawables( boolean isForOutput )
    {
        super();
        isOutputBuf          = isForOutput;
        if ( isOutputBuf ) {
            buf4nestable       = new ArrayList();
            buf4nestless       = new ArrayList();
        }
        else {
            buf4nestable       = null;
            buf4nestless       = null;
        }

        haveObjectsBeenSaved = false;
        total_bytesize       = INIT_BYTESIZE;
    }

    public int getByteSize()
    {
        return total_bytesize;
    }

    public void add( final Primitive prime )
    {
        if ( prime.getCategory().getTopology().isState() )
            buf4nestable.add( prime );
        else
            buf4nestless.add( prime );
        // Extra 1 byte indicates if the Drawable is Primitive/Composite
        total_bytesize += ( prime.getByteSize() + 1 );
    }

    public void add( final Composite cmplx )
    {
        // assume all Composites contain state-like object
        buf4nestable.add( cmplx );
        // Extra 1 byte indicates if the Drawable is Primitive/Composite
        total_bytesize += ( cmplx.getByteSize() + 1 );
    }

    public void empty()
    {
        if ( haveObjectsBeenSaved ) {
            buf4nestable.clear();
            buf4nestless.clear();
            haveObjectsBeenSaved = false;
            total_bytesize       = INIT_BYTESIZE;
        }
    }

    public int getNumOfDrawables()
    {
        return buf4nestable.size() + buf4nestless.size();
    }

    /*  This is expensive compared to getNumOfDrawables, avoid calling this  */
    public int getNumOfPrimitives()
    {
        int       count;
        Iterator  dobjs_itr;
        // assume buf4nestless contains only primitives, e.g. arrow/event
        count     = buf4nestless.size();
        dobjs_itr = buf4nestable.iterator();
        while ( dobjs_itr.hasNext() )
            count += ( (Drawable) dobjs_itr.next() ).getNumOfPrimitives();
        return count;
    }

    // Iterator of Nestable Drawables in Increasing StartTime order
    public Iterator nestableForeIterator( final TimeBoundingBox tframe,
                                                boolean         isComposite )
    {
        if ( isComposite )
            return new IteratorOfForeDrawables( buf4nestable, tframe );
        else
            return new IteratorOfForePrimitives( buf4nestable, tframe );
    }
    
    // Iterator of Nestless Drawables in Increasing StartTime order
    public Iterator nestlessForeIterator( final TimeBoundingBox tframe )
    {
        return new IteratorOfForeDrawables( buf4nestless, tframe );
    }

    // Iterator of Nestable Drawables in Decreasing StartTime order
    public Iterator nestableBackIterator( final TimeBoundingBox tframe,
                                                boolean         isComposite )
    {
        if ( isComposite )
            return new IteratorOfBackDrawables( buf4nestable, tframe );
        else
            return new IteratorOfBackPrimitives( buf4nestable, tframe );
    }

    // Iterator of Nestless Drawables in Decreasing StartTime order
    public Iterator nestlessBackIterator( final TimeBoundingBox tframe )
    {
        return new IteratorOfBackDrawables( buf4nestless, tframe );
    }

    public LineIDMap getIdentityLineIDMap()
    {
        List buf4drawables = new ArrayList( buf4nestable );
        buf4drawables.addAll( buf4nestless );
        return super.toIdentityLineIDMap( buf4drawables );
    }


    public void writeObject( MixedDataOutput outs )
    throws java.io.IOException
    {
        ListIterator dobjs_itr;
        Drawable     dobj;
        int          Nobjs;

        super.writeObject( outs );   // BufForObjects.writeObject( outs )

        // Save the Lists in Increasing Starttime order
        Collections.sort( buf4nestable, DRAWING_ORDER );
        Collections.sort( buf4nestless, DRAWING_ORDER );

        // assume buf4nestless contains only primitives, e.g. arrow/event
        Nobjs  = buf4nestless.size();
        outs.writeInt( Nobjs );
        dobjs_itr = buf4nestless.listIterator( 0 );
        while ( dobjs_itr.hasNext() ) {
            dobj = (Drawable) dobjs_itr.next();
            if ( dobj instanceof Composite ) {
                outs.writeByte( (int) COMPOSITE_ID );
                ( (Composite) dobj ).writeObject( outs );
            }
            else {
                outs.writeByte( (int) PRIMITIVE_ID );
                ( (Primitive) dobj ).writeObject( outs );
            }
        }

        // assume buf4nestable contains both primitives and composites.
        Nobjs  = buf4nestable.size();
        outs.writeInt( Nobjs );
        dobjs_itr = buf4nestable.listIterator( 0 );
        while ( dobjs_itr.hasNext() ) {
            dobj = (Drawable) dobjs_itr.next();
            if ( dobj instanceof Composite ) {
                outs.writeByte( (int) COMPOSITE_ID );
                ( (Composite) dobj ).writeObject( outs );
            }
            else {
                outs.writeByte( (int) PRIMITIVE_ID );
                ( (Primitive) dobj ).writeObject( outs );
            }
        }

        haveObjectsBeenSaved = true;
    }

    public BufForDrawables( MixedDataInput ins, final Map categorymap )
    throws java.io.IOException
    {
        this( false );
        this.readObject( ins, categorymap );
    }

    public void readObject( MixedDataInput ins, final Map categorymap )
    throws java.io.IOException
    {
        Primitive  prime;
        Composite  cmplx;
        byte       dobj_type;
        int        Nobjs, idx;

        super.readObject( ins );   // BufForObjects.readObject( ins )
            
        // assume buf4nestless contains only primitives, e.g. arrow/event
        Nobjs = ins.readInt();
        buf4nestless = new ArrayList( Nobjs );
        for ( idx = 0; idx < Nobjs; idx++ ) {
            dobj_type = ins.readByte();
            switch ( dobj_type ) {
                case PRIMITIVE_ID:
                    prime = new Primitive( ins );
                    prime.resolveCategory( categorymap );
                    buf4nestless.add( prime );
                    total_bytesize += ( prime.getByteSize() + 1 );
                    break;
                case COMPOSITE_ID:
                    cmplx = new Composite( ins );
                    cmplx.resolveCategory( categorymap );
                    buf4nestless.add( cmplx );
                    total_bytesize += ( cmplx.getByteSize() + 1 );
                    break;
                default:
                    System.err.println( "BufForDrawables: Error! "
                                      + "Unknown drawable type = "
                                      + dobj_type );
            }
        }

        // assume buf4nestable contains both primitives and composites.
        Nobjs = ins.readInt();
        buf4nestable = new ArrayList( Nobjs );
        for ( idx = 0; idx < Nobjs; idx++ ) {
            dobj_type = ins.readByte();
            switch ( dobj_type ) {
                case PRIMITIVE_ID:
                    prime = new Primitive( ins );
                    prime.resolveCategory( categorymap );
                    buf4nestable.add( prime );
                    total_bytesize += ( prime.getByteSize() + 1 );
                    break;
                case COMPOSITE_ID:
                    cmplx = new Composite( ins );
                    cmplx.resolveCategory( categorymap );
                    buf4nestable.add( cmplx );
                    total_bytesize += ( cmplx.getByteSize() + 1 );
                    break;
                default:
                    System.err.println( "BufForDrawables: Error! "
                                      + "Unknown drawable type = "
                                      + dobj_type );
            }
        }
    }

    public String toString()
    {
        Drawable dobj;
        Iterator dobjs_itr;
        int      idx;

        StringBuffer rep = new StringBuffer( "    BufForDrawables{ " );
        rep.append( super.toString() /* BufForObjects */ );
        rep.append( " }\n" );

        dobjs_itr = new ForeItrOfDobjs( this );
        for ( idx = 1; dobjs_itr.hasNext(); idx++ )
            rep.append( idx + ": " + dobjs_itr.next() + "\n" );

        return rep.toString();
    }



    /*
       Iterators to return Drawables (Primitive/Composite)
       in Increasing StartTime Order(1st) and then Decreasing EndTime Order(2nd)
    */
    private class ForeItrOfDobjs implements Iterator
    {
        private TimeBoundingBox  timeframe;
        private Iterator         nestable_itr;
        private Iterator         nestless_itr;
        private Drawable         nestable_dobj;
        private Drawable         nestless_dobj;
        private Drawable         next_drawable;

        public ForeItrOfDobjs( final TimeBoundingBox  tframe )
        {
            timeframe      = tframe;
            nestable_itr   = new IteratorOfForeDrawables( buf4nestable,
                                                          tframe );
            nestless_itr   = new IteratorOfForeDrawables( buf4nestless,
                                                          tframe );
            nestable_dobj  = null;
            nestless_dobj  = null;
            next_drawable  = null;
        }

        public boolean hasNext()
        {
            double nestable_starttime, nestless_starttime;

            if ( nestable_dobj == null ) {
                if ( nestable_itr.hasNext() )
                    nestable_dobj = (Drawable) nestable_itr.next();
            }

            if ( nestless_dobj == null ) {
                if ( nestless_itr.hasNext() )
                    nestless_dobj = (Drawable) nestless_itr.next();
            }

            if ( nestable_dobj != null && nestless_dobj != null ) {
                nestable_starttime = nestable_dobj.getEarliestTime();
                nestless_starttime = nestless_dobj.getEarliestTime();
                if ( nestable_starttime == nestless_starttime ) {
                    if ( nestable_dobj.getLatestTime()
                       < nestless_dobj.getLatestTime() ) {
                        next_drawable = nestless_dobj;
                        nestless_dobj = null;
                        return true;
                    }
                    else {
                        next_drawable = nestable_dobj;
                        nestable_dobj = null;
                        return true;
                    }
                }
                else { /* Significant Order */
                    if ( nestable_starttime < nestless_starttime ) {
                        next_drawable = nestable_dobj;
                        nestable_dobj = null;
                        return true;
                    }
                    else {
                        next_drawable = nestless_dobj;
                        nestless_dobj = null;
                        return true;
                    }
                }
            }

            if ( nestable_dobj != null ) {
                next_drawable = nestable_dobj;
                nestable_dobj = null;
                return true;
            }

            if ( nestless_dobj != null ) {
                next_drawable = nestless_dobj;
                nestless_dobj = null;
                return true;
            }

            return false;
        }

        public Object next()
        {
            return next_drawable;
        }

        public void remove() {}
    }   // private class ForeItrOfDobjs

}

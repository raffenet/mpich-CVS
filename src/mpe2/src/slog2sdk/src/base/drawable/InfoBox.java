/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.drawable;

import java.util.Map;
import java.io.ByteArrayInputStream;
import java.io.DataInput;
import java.io.DataOutput;

import base.io.MixedDataInputStream;

public class InfoBox extends TimeBoundingBox
{
    private static final int INIT_BYTESIZE = 4  /* type_idx */
                                           + 2  /* infobuffer.length */ ; 
    private static final int INVALID_INDEX = Integer.MIN_VALUE;

    private   int          type_idx;       // tmp variable before type is known
    private   Category     type;
    private   boolean      hasBufBeenDecoded;
    private   byte[]       infobuffer;     // byte[] passed down by TRACE-API
    private   InfoValue[]  infovals;       // InfoValue[] for InfoKey[]

    public InfoBox()
    {
        super();
        this.initCategoryToNull();
    }

    //  For support of Trace API's  Primitive/Composite generation
    public InfoBox( int in_type_idx )
    {
        super();
        this.initCategoryToNull();
        type_idx  = in_type_idx;
    }

    public InfoBox( final Category in_type )
    {
        super();
        this.setCategory( in_type );
    }

    //  This is NOT a copy constructor,
    //  only Category and InfoType[] are copied, not InfoValue[].
    public InfoBox( final InfoBox infobox )
    {
        super( infobox );  // TimeBoundingBox( TimeBoundingBox );
        this.setCategory( infobox.type );
    }

    public InfoBox( Category in_type, final InfoBox infobox )
    {
        super( infobox );  // TimeBoundingBox( TimeBoundingBox );
        this.setCategory( in_type );
    }

    private void initCategoryToNull()
    {
        this.type              = null;
        this.type_idx          = INVALID_INDEX;
        this.infobuffer        = null;
        this.infovals          = null;
        this.hasBufBeenDecoded = false;
    }

    private void setCategory( final Category in_type )
    {
        if ( in_type != null ) {
            this.type              = in_type;
            this.type_idx          = type.getIndex();
            this.infobuffer        = null;
            this.infovals          = null;
            this.hasBufBeenDecoded = false;
            // this.setInfoValueTypes();
        }
        else 
            this.initCategoryToNull();
    }

    //  For SLOG-2 Input and Output APIs
    public boolean resolveCategory( final Map categorymap )
    {
        if ( type == null ) {
            // For Output: TraceTOslog2. For Input: BufForDrawables.readObject
            if ( type_idx != INVALID_INDEX ) {
                type = (Category) categorymap.get( new Integer( type_idx ) );
                if ( type != null ) {
                    type.setUsed( true );  // set Category.hasBeenUsed to TRUE
                    return true;
                }
            }
        }
        else {
            // For Output: ClogToSlog2
            type.setUsed( true );
            return true;
        }
        return false;
    }

    public Category getCategory()
    {
        return type;
    }

    public int getCategoryIndex()
    {
        return type_idx;
    }



    public void setInfoBuffer( final byte[] byte_infovals )
    {
        this.infobuffer  = byte_infovals;
    }

    private void setInfoValueTypes()
    {
        if ( type != null ) {
            InfoType[]  infotypes;
            infotypes        = type.getInfoTypes();
            if ( infotypes != null && infotypes.length > 0 ) {
                int infotypes_length = infotypes.length;
                infovals  = new InfoValue[ infotypes_length ];
                // Set InfoValue[i]'s type first
                // fill content later with this.setInfoValues( Byte[] )
                for ( int idx = 0; idx < infotypes_length; idx++ )
                    infovals[ idx ] = new InfoValue( infotypes[ idx ] );
            }
            else
                infovals  = null;
        }
    }

    private void setInfoValues()
    {
        ByteArrayInputStream  bary_ins;
        MixedDataInputStream  data_ins;

        if ( this.infobuffer == null )
            return;

        this.setInfoValueTypes();

        bary_ins = new ByteArrayInputStream( this.infobuffer );
        data_ins = new MixedDataInputStream( bary_ins );
        try {
            for ( int idx = 0; idx < infovals.length; idx++ )
                infovals[ idx ].readValue( data_ins );
            data_ins.close();
            bary_ins.close();
        } catch ( java.io.IOException ioerr ) {
            ioerr.printStackTrace();
            System.exit( 1 );
        }
    }

    private void decodeInfoBuffer()
    {
        if ( ! this.hasBufBeenDecoded ) {
            this.hasBufBeenDecoded = true;
            this.setInfoValues();
        }
    }

    private String[]  infokeys = null;

    // for slog2print. External handle to get a hold of InfoBuffer's content
    public int    getInfoLength()
    {
        int      max_length, infokeys_length, infovals_length;

        this.decodeInfoBuffer();
        if ( type != null ) {
            infokeys = type.getInfoKeys();
            if (    ( infokeys != null && infokeys.length > 0 )
                 || ( infovals != null && infovals.length > 0 ) ) {
                if ( infokeys != null )
                    infokeys_length = infokeys.length;
                else
                    infokeys_length = 0;
                if ( infovals != null )
                    infovals_length = infovals.length;
                else
                    infovals_length = 0;
                max_length = Math.max( infokeys_length, infovals_length );
                return max_length;
            }
         }
         return 0;
    }

    // for slog2print. External handle to get a hold of InfoBuffer's content
    public String getInfoKey( int idx )
    {
        this.decodeInfoBuffer();
        if ( infokeys != null && idx < infokeys.length )
            return infokeys[ idx ];
        if ( type != null ) {
            infokeys = type.getInfoKeys();
            if ( infokeys != null && idx < infokeys.length )
                return infokeys[ idx ];
        }
        return null;
    }

    // for slog2print. External handle to get a hold of InfoBuffer's content
    public InfoValue getInfoValue( int idx )
    {
        this.decodeInfoBuffer();
        if ( infovals != null && idx < infovals.length )
            return infovals[ idx ];
        else
            return null;
    }



    public int getByteSize()
    {
        int bytesize;
        bytesize = INIT_BYTESIZE;
        if ( infobuffer != null )
            bytesize += infobuffer.length;
        return bytesize;
    }

    public void writeObject( DataOutput outs )
    throws java.io.IOException
    {
        outs.writeInt( type_idx );

        if ( infobuffer != null && infobuffer.length > 0 ) {
            outs.writeShort( infobuffer.length );
            outs.write( infobuffer );
        }
        else
            outs.writeShort( 0 );
    }

    public InfoBox( DataInput ins )
    throws java.io.IOException
    {
        super();     // TimeBoundingBox();
        this.readObject( ins );
    }

    public void readObject( DataInput ins )
    throws java.io.IOException
    {
        short infobuf_length;

        type_idx   = ins.readInt();

        infobuf_length = ins.readShort();
        if ( infobuf_length > 0 ) {
            infobuffer = new byte[ infobuf_length ];
            ins.readFully( infobuffer );
        }
        else
            infobuffer = null;    
    }

/*
    public void setInfoValueContent( int idx, final Object content )
    {
        infovals[ idx ].setValue( content );
    }

    public Object getInfoValueContent( int idx )
    {
        return infovals[ idx ].getValue();
    }
*/

    public String toInfoBoxString()
    {
        StringBuffer  rep;
        int           max_length, infokeys_length, infovals_length;

        rep = new StringBuffer();
        this.decodeInfoBuffer();
        if ( type != null ) {
            infokeys = type.getInfoKeys();
            if (    ( infokeys != null && infokeys.length > 0 )
                 || ( infovals != null && infovals.length > 0 ) ) {
                if ( infokeys != null )
                    infokeys_length = infokeys.length;
                else
                    infokeys_length = 0;
                if ( infovals != null )
                    infovals_length = infovals.length;
                else
                    infovals_length = 0;
                max_length = Math.max( infokeys_length, infovals_length );

                for ( int idx = 0; idx < max_length; idx++ ) {
                    if ( idx < infokeys_length )
                        rep.append( infokeys[ idx ] );
                    if ( idx < infovals_length )
                        rep.append( infovals[ idx ] );
                }
            }
        }
        else {
            if ( infovals != null ) {
                for ( int idx = 0; idx < infovals.length; idx++ )
                    rep.append( infovals[ idx ] + " " );
            }
        }
        return rep.toString();
    }

    public String toString()
    {
        StringBuffer  rep;
        int           max_length, infokeys_length, infovals_length;

        rep = new StringBuffer( "infobox[ " + super.toString() + " " );
        if ( type != null )
            rep.append( "Category=" + type + ": " );
        else
            rep.append( "Category=" + type_idx + ": " );

        this.decodeInfoBuffer();
        if ( type != null ) {
            infokeys = type.getInfoKeys();
            if (    ( infokeys != null && infokeys.length > 0 )
                 || ( infovals != null && infovals.length > 0 ) ) {
                if ( infokeys != null )
                    infokeys_length = infokeys.length;
                else
                    infokeys_length = 0;
                if ( infovals != null )
                    infovals_length = infovals.length;
                else
                    infovals_length = 0;
                max_length = Math.max( infokeys_length, infovals_length );

                for ( int idx = 0; idx < max_length; idx++ ) {
                    if ( idx < infokeys_length )
                        rep.append( infokeys[ idx ] );
                    if ( idx < infovals_length )
                        rep.append( infovals[ idx ] );
                }
            }
        }
        else {
            if ( infovals != null ) {
                for ( int idx = 0; idx < infovals.length; idx++ )
                    rep.append( infovals[ idx ] + " " );
            }
        }

        rep.append( "]" );
        return rep.toString();
    }
}

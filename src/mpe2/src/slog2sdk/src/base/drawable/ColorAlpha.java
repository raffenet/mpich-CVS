/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.drawable;

import java.awt.Color;
import java.io.DataInput;
import java.io.DataOutput;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;

public class ColorAlpha extends Color
{
    public static final int         BYTESIZE         = 5;

    public static final int         OPAQUE           = 255;
    public static final int         NEAR_OPAQUE      = 191;
    public static final int         HALF_OPAQUE      = 127;
    public static final int         NEAR_TRANSPARENT = 63;
    public static final int         TRANSPARENT      = 0;

    private boolean  isModifiable;

    public ColorAlpha()
    {
        // {red=255, green=192, blue= 203} is "pink" from jumpshot.colors
        // Initialize all color to "pink", and "opaque"
        super( 255, 192, 203, ColorAlpha.OPAQUE );
        isModifiable  = true;
    }

    public ColorAlpha( int red, int green, int blue )
    {
        super( red, green, blue, ColorAlpha.OPAQUE );
        isModifiable  = true;
    }

    public ColorAlpha( Color color, int alpha )
    {
        super( color.getRed(), color.getGreen(), color.getBlue(), alpha );
        isModifiable  = true;
    }

    public ColorAlpha( int red, int green, int blue, int alpha,
                       boolean in_isModifiable )
    {
        super( red, green, blue, alpha );
        isModifiable  = in_isModifiable;
    }

    public void writeObject( DataOutput outs )
    throws java.io.IOException
    {
        outs.writeInt( super.getRGB() );
        outs.writeBoolean( isModifiable );
    }

    public ColorAlpha( DataInput ins )
    throws java.io.IOException
    {
        super( ins.readInt(), true );
        isModifiable  = ins.readBoolean();
    }
 
    // Since java.awt.Color cannot be altered after creation.
    // readObject( DataInput ) cannot be done/used.
    public void readObject( DataInput ins )
    throws java.io.IOException
    {
        System.err.println( "ColorAlpha.readObject() should NOT called" );
    }

    public String toString()
    {
        return "(" + getRed() + "," + getGreen() +  "," + getBlue() 
             + "," + getAlpha() + "," + isModifiable + ")";
    }

    public static final void main( String[] args )
    {
        final String filename    = "tmp_color.dat";
 
        String   io_str = args[ 0 ].trim();
        boolean  isWriting = io_str.equals( "write" );

        ColorAlpha colorX = null;

        if ( isWriting ) {
            colorX = new ColorAlpha( 10, -1, 30, 100, false );
            try {
                FileOutputStream fout = new FileOutputStream( filename );
                DataOutputStream dout = new DataOutputStream( fout );
                colorX.writeObject( dout );
                fout.close();
            } catch ( java.io.IOException ioerr ) {
                ioerr.printStackTrace();
                System.exit( 1 );
            }
            System.out.println( "ColorAlpha " + colorX
                              + " has been written to " + filename );
        }
        else {
            try {
                FileInputStream fin   = new FileInputStream( filename );
                DataInputStream din   = new DataInputStream( fin );
                colorX = new ColorAlpha( din );
                fin.close();
            } catch ( java.io.IOException ioerr ) {
                ioerr.printStackTrace();
                System.exit( 1 );
            }
            System.out.println( "ColorAlpha " + colorX
                              + " has been read from " + filename );
        }
    }
}

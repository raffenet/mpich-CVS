/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */
package base.io;

import java.io.InputStream;
import java.io.DataInputStream;

public class MixedDataInputStream extends DataInputStream
                                  implements MixedDataInput
{
    public MixedDataInputStream( InputStream ins )
    {
        super( ins );
    }

    public String readString()
    throws java.io.IOException
    {
        short strlen = super.readShort();
        if ( strlen > 0 ) {
            byte[] bytebuf = new byte[ strlen ];
            super.readFully( bytebuf );
            // return ( new String( bytebuf ) ).trim();
            return ( new String( bytebuf ) );
        }
        else
            return "";
    }
}

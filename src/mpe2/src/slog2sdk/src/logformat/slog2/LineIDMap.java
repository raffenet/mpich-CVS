/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.slog2;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.List;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Collections;
import java.util.Map;
import java.util.TreeMap;
import java.util.Iterator;

import base.drawable.YCoordMap;
import base.drawable.Method;
import base.io.MixedDataInput;
import base.io.MixedDataOutput;
import base.io.MixedDataInputStream;
import base.io.MixedDataOutputStream;

/*
    LineIDMap is the mapping between lineID and YaxisHierarchy.
    lineID is the basically the primitive Yaxis label integer
    YaxisHierarchy is the YaxisTreePath, i.e. list of YaxisTreeNode

    LineIDMap's key   is of type Integer
    LineIDMap's value is of type Integer[ num_treenodes ]
*/
public class LineIDMap extends TreeMap
{
    private int       num_treenodes;
    private String    title_label;
    private String[]  column_labels;
    private Method[]  methods;

    public LineIDMap()
    {
        super();
        num_treenodes = 0;
        title_label   = null;
        column_labels = null;
        methods       = null;
    }

    public LineIDMap( int Ntreenodes )
    {
        super();
        num_treenodes = Ntreenodes;
        title_label   = null;
        column_labels = null;
        methods       = null;
    }

    public LineIDMap( final YCoordMap  y_map )
    {
        super();
        num_treenodes = y_map.getNumOfColumns() - 1;
        title_label   = y_map.getTitleName();
        column_labels = y_map.getColumnNames();
        methods       = y_map.getMethods();

        Integer    key;
        Integer[]  value;   // LineIDNodePath
        int        idx;
        int[]      map_elems;
        int        num_entries;
        int        irow, icol;
        if ( column_labels.length != num_treenodes ) {
            System.err.print( "Warning: The input column_labels[] is " );
            for ( icol = 0; icol < column_labels.length; icol++ )
                System.err.print( column_labels[ icol ] + " " );
            System.err.println();
            System.err.println( "The number of column labels is expected "
                              + "to be " + num_treenodes + "." );
        }
        num_entries = y_map.getNumOfRows();
        map_elems   = y_map.getMapElems();
        if ( map_elems.length != num_entries * ( num_treenodes + 1 ) ) {
            System.err.println( "Warning: The input YCoordMap contains "
                              + map_elems.length + " elements.  "
                              + "But the number is expected to be "
                              + num_entries * ( num_treenodes + 1 ) + "." );
        }
        idx = 0;
        for ( irow = 0; irow < num_entries; irow++ ) {
            key    = new Integer( map_elems[ idx++ ] );
            value  = new Integer[ num_treenodes ];
            for ( icol = 0; icol < num_treenodes; icol++ )
                value[ icol ] = new Integer( map_elems[ idx++ ] );
            super.put( key, value );
        }
    }

    public Method[] getMethods()
    {
        return methods;
    }

    public void setTitle( String title_labelname )
    {
        title_label = title_labelname;
    }

    public String getTitle()
    {
        return title_label;
    }

    public void setColumnLabels( String[] col_labels )
    {
        if ( col_labels.length != num_treenodes ) {
            System.err.print( "Warning: The input column_labels[] is " );
            for ( int idx = 0; idx < col_labels.length; idx++ )
                System.err.print( col_labels[ idx ] + " " );
            System.err.println();
            System.err.println( "The number of column labels is expected "
                              + "to be " + num_treenodes + "." );
        }
        column_labels = col_labels;
    }

    public String[] getColumnLabels()
    {
        return column_labels;
    }

    public void writeObject( MixedDataOutput outs )
    throws java.io.IOException
    {
        Map.Entry  entry;
        Integer    key;
        Integer[]  value;   // LineIDNodePath;
        int        idx;

        outs.writeString( title_label );
        outs.writeInt( num_treenodes );
        for ( idx = 0; idx < num_treenodes; idx++ )
            outs.writeString( column_labels[ idx ] );
        outs.writeInt( super.size() );
        Iterator entries = this.entrySet().iterator();
        while ( entries.hasNext() ) {
            entry = (Map.Entry) entries.next();
            key   = (Integer)   entry.getKey();
            value = (Integer[]) entry.getValue();
            outs.writeInt( key.intValue() );
            for ( idx = 0; idx < num_treenodes; idx++ )
                outs.writeInt( value[ idx ].intValue() );
        }

        if ( methods != null && methods.length > 0 ) {
            outs.writeShort( (short) methods.length );
            for ( idx = 0; idx < methods.length; idx++ )
                methods[ idx ].writeObject( outs );
        }
        else
            outs.writeShort( 0 );
    }

    public LineIDMap( MixedDataInput ins )
    throws java.io.IOException
    {
        this();
        this.readObject( ins );
    }

    public void readObject( MixedDataInput ins )
    throws java.io.IOException
    {
        Integer    key;
        Integer[]  value;   // LineIDNodePath
        int        Nentries;
        int        idx, ientry;

        title_label    = ins.readString();
        num_treenodes  = ins.readInt();
        column_labels  = new String[ num_treenodes ];
        for ( idx = 0; idx < num_treenodes; idx++ )
            column_labels[ idx ] = ins.readString();
        Nentries   = ins.readInt();
        for ( ientry = 0; ientry < Nentries; ientry++ ) {
            key    = new Integer( ins.readInt() );
            value  = new Integer[ num_treenodes ];
            for ( idx = 0; idx < num_treenodes; idx++ )
                value[ idx ] = new Integer( ins.readInt() );
            super.put( key, value );
        }

        Nentries = (int) ins.readShort();
        if ( Nentries > 0 ) {
            methods = new Method[ Nentries ];
            for ( idx = 0; idx < Nentries; idx++ )
                methods[ idx ]   = new Method( ins );
        }
        else
            methods   = null;
    }

    public String toString()
    {
        Map.Entry  entry;
        Integer    key;
        Integer[]  value;   // LineIDNodePath;
        int        ientry;
        int        idx;

        StringBuffer rep = new StringBuffer( "Title: " + title_label + "\n" );
        rep.append( "Column Labels: LineID -> " );
        for ( idx = 0; idx < num_treenodes; idx++ )
            rep.append( column_labels[ idx ] + " " );
        rep.append( "\n" );
        Iterator entries = this.entrySet().iterator();
        ientry = 0;
        while ( entries.hasNext() ) {
            ientry++;
            entry = (Map.Entry) entries.next();
            key   = (Integer) entry.getKey();
            value = (Integer[]) entry.getValue();
            rep.append( ientry + ", " + key + ":  " );
            for ( idx = 0; idx < num_treenodes; idx++ )
                rep.append( value[ idx ] + " " );
            rep.append( "\n" );
        }
        if ( methods != null && methods.length > 0 ) {
            rep.append( "methods=< " );
            for ( idx = 0; idx < methods.length; idx++ )
                rep.append( methods[ idx ] + " " );
            rep.append( ">\n" );
        }
        return rep.toString();
    }

    /*
        Invoke like
        java LineIDMap write 3 2   for writing file
        java LineIDMap read        for reading file
    */
    public final static void main( String[] args )
    {
        final String  filename            = "tmp_LineIDMap.dat";

        String     io_str = args[ 0 ].trim();
        boolean    isWriting = io_str.equals( "write" );

        int        Nlevels, Nchilds, Nentries;
        int[]      icfg;
        Integer[]  ocfg;
        List       cfglist = new ArrayList();
        Iterator   cfgs;
        LineIDMap  linemap = null;

        if ( isWriting ) {
            Nlevels = Integer.parseInt( args[ 1 ] );
            Nchilds = Integer.parseInt( args[ 2 ] );
            Enumeration perms = new Permutation( Nlevels, Nchilds );
            while ( perms.hasMoreElements() ) {
                icfg = (int[]) perms.nextElement();
                cfglist.add( icfg );
                cfglist.add( icfg.clone() );
            }
            Collections.shuffle( cfglist );

            linemap = new LineIDMap( Nlevels );
            linemap.setTitle( "SLOG2 LineIDMap sample" );
            String[] col_names = new String[ Nlevels ];
            for ( int idx = 0; idx < Nlevels; idx++ )
                col_names[ idx ] = "column" + idx;
            linemap.setColumnLabels( col_names );
            cfgs = cfglist.iterator();
            for ( int cfg_idx = 1; cfgs.hasNext(); cfg_idx++ ) {
                icfg = (int[]) cfgs.next();
                ocfg = new Integer[ icfg.length ];
                for ( int idx = 0 ; idx < ocfg.length; idx++ )
                     ocfg[ idx ] = new Integer( icfg[ idx ] );
                linemap.put( new Integer( 10000 + cfg_idx ), ocfg );
            }
            System.out.println( "LineIDMap:\n" + linemap );

            try {
                FileOutputStream      fout = new FileOutputStream( filename );
                MixedDataOutputStream dout = new MixedDataOutputStream( fout );
                linemap.writeObject( dout );
                fout.close();
            } catch ( java.io.IOException ioerr ) {
                ioerr.printStackTrace();
                System.exit( 1 );
            }
        }
        else {
            try {
                FileInputStream      fin   = new FileInputStream( filename );
                MixedDataInputStream din   = new MixedDataInputStream( fin );
                linemap = new LineIDMap( din );
                fin.close();
            } catch ( java.io.IOException ioerr ) {
                ioerr.printStackTrace();
                System.exit( 1 );
            }
            System.out.println( linemap );
        }
    }
}

/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.statistics;

import java.util.Map;
import java.util.HashMap;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Collections;

import base.drawable.TimeBoundingBox;
import base.drawable.Topology;
import base.drawable.Category;
import base.drawable.Drawable;
import base.drawable.Primitive;
import base.drawable.Shadow;

public class BufForTimeAveBoxes extends TimeBoundingBox
{
    private TimeBoundingBox   timebounds;
    private Map               map_vtxs2nestable;   /* state and composite */
    private Map               map_vtxs2nestless;   /* arrow/event */

    public BufForTimeAveBoxes( final TimeBoundingBox timebox )
    {
        super( timebox );
        timebounds         = timebox;
        map_vtxs2nestable  = new HashMap();
        map_vtxs2nestless  = new HashMap();
    }

    public void merge( final Drawable dobj )
    {
        List        key;
        Topology    topo;
        TimeAveBox  avebox;
        boolean     isNestable;

        key = new ArrayList();
        topo = dobj.getCategory().getTopology();
        key.add( topo );
        // key.addAll( prime.getListOfVertexLineIDs() );
        key.add( new Integer( dobj.getStartVertex().lineID ) );
        key.add( new Integer( dobj.getFinalVertex().lineID ) );
        avebox      = null;
        isNestable  = topo.isState();
        if ( isNestable )
            avebox = (TimeAveBox) map_vtxs2nestable.get( key );
        else
            avebox = (TimeAveBox) map_vtxs2nestless.get( key );
        if ( avebox == null ) {
            avebox = new TimeAveBox( timebounds );
            if ( isNestable )
                map_vtxs2nestable.put( key, avebox );
            else
                map_vtxs2nestless.put( key, avebox );
        }
        if ( dobj instanceof Shadow )
            avebox.mergeWithShadow( (Shadow) dobj );
        else
            avebox.mergeWithReal( dobj );
    }

    public String toString()
    {
        StringBuffer rep = new StringBuffer( super.toString() );
        
        if ( map_vtxs2nestable.size() > 0 ) {
            Map.Entry  entry;
            Object[]   key;
            TimeAveBox avebox;
            Iterator   entries  = map_vtxs2nestable.entrySet().iterator();
            while ( entries.hasNext() ) {
                entry  = (Map.Entry) entries.next();
                key    = ( (List) entry.getKey() ).toArray();
                avebox = (TimeAveBox) entry.getValue();
                rep.append( "\n" + key[0] + ": " + key[1] + ", " + key[2] );
                rep.append( "\n" + avebox );
            }
            rep.append( "\n" );
        }

        if ( map_vtxs2nestless.size() > 0 ) {
            Map.Entry  entry;
            Object[]   key;
            TimeAveBox avebox;
            Iterator   entries  = map_vtxs2nestless.entrySet().iterator();
            while ( entries.hasNext() ) {
                entry  = (Map.Entry) entries.next();
                key    = ( (List) entry.getKey() ).toArray();
                avebox = (TimeAveBox) entry.getValue();
                rep.append( "\n" + key[0] + ": " + key[1] + ", " + key[2] );
                rep.append( "\n" + avebox );
            }
            rep.append( "\n" );
        }

        return rep.toString();
    }
}

package logformat.slog2.input;

import java.util.Comparator;
import base.drawable.TimeBoundingBox;

public class CmptrOfFinalTime implements Comparator
{
    // Sort TimeBoundingBox's Latest Time in Increasing order
    public int compare( Object o1, Object o2 )
    {
        double endtime1, endtime2;
        endtime1 = ( (TimeBoundingBox) o1 ).getLatestTime();
        endtime2 = ( (TimeBoundingBox) o2 ).getLatestTime();
        return endtime1 < endtime2 ? -1 : ( endtime1 == endtime2 ? 0 : 1 );
    }
}

#strict 2

/* ---- Colors ---- */

static const RGBA_ALPHA = 0;
static const RGBA_RED = 1;
static const RGBA_GREEN = 2;
static const RGBA_BLUE = 3;

global func HSL(int h, int s, int l)  { return HSL2RGB(RGB(h,s,l)); }
global func HSLa(int h, int s, int l, int a) { return  HSL2RGB(RGB(h,s,l)) | (a & 255)<<24; }

global func RGB(int r, int g, int b) { return (r & 255)<<16 | (g & 255)<<8 | (b & 255); }
global func RGBa (int r, int g, int b, int a) { return (a & 255)<<24 | (r & 255)<<16 | (g & 255)<<8 | (b & 255); }

global func GetRGBaValue(int val, int sel) { return (val>>((3-sel)*8)&255); }
global func DoRGBaValue(int val, int chng, int sel) { return (val + (chng<<((3-sel)*8))); }
global func SetRGBaValue(int val, int newval, int sel) {
    // 'delete' old color
    val = val&~(255<<((3-sel)*8));
    // add new
    return (val|newval<<((3-sel)*8));
}
global func SplitRGBaValue(rgb, &red, &green, &blue, &alpha) {
    red=GetRGBaValue(rgb,1);
    green=GetRGBaValue(rgb,2);
    blue=GetRGBaValue(rgb,3);
    alpha=GetRGBaValue(rgb,0);
}

global func HSL2RGB(hsl) {
    var hue=GetRGBaValue(hsl,1), sat=GetRGBaValue(hsl,2),lightness=GetRGBaValue(hsl,3);
    var red, green, blue;
    var var1, var2;
    
    //Log("hue: %d sat: %d lightness: %d",hue,sat, lightness);
    
    if(sat==0) {
        red = green = blue = lightness;
    } else {
        if(lightness<128) var2 = (lightness*(255 + sat))/255;
            else var2 = lightness+sat-lightness*sat/255;
                
        var1 = 2*lightness-var2;
            
        red   = Hue_2_RGB( var1, var2, hue+85);
        green= Hue_2_RGB( var1, var2, hue );
        blue  = Hue_2_RGB( var1, var2, hue-85);
    }
    
    //Log("red: %d green: %d blue: %d",red, green, blue);
    
    return RGB(red, green, blue);
}

global func Hue_2_RGB(var1, var2, hue) {
   if(hue<0) hue+=255;
   if(hue>255) hue-=255;
   if(6*hue<255) return ( var1 + ((var2 - var1) * 6 * hue)/255);
   if(2*hue<255) return ( var2 );
   if(3*hue<510) return ( var1 + ((var2 - var1)*( 510 / 3 - hue )*6)/255);
   return (var1);
}

global func RGB2HSL(rgb) {
    var red=GetRGBaValue(rgb,1), green=GetRGBaValue(rgb,2),blue=GetRGBaValue(rgb,3);
    var min_val = Min(red, Min(green, blue)), max_val = Max(red, Max(green, blue));
    var diff_val = max_val - min_val;
    var lightness = (max_val + min_val)/2;
    var hue, sat, diff_red, diff_green, diff_blue;

    //Log("red: %d green: %d blue: %d",red, green, blue);
    //Log("max_val: %d, min_val: %d",max_val, min_val);
    
    if (diff_val==0) {
   hue=0;                             
   sat=0;
    } else {
        //Log("%d/%d",255*diff_val,510-(max_val+min_val));
        if(lightness<128) sat=(255*diff_val)/(max_val+min_val);
            else sat=(255*diff_val)/(510-(max_val+min_val));

        diff_red  = ((255*(max_val-red  ))/6 + (255*diff_val)/2)/diff_val;
        diff_green= ((255*(max_val-green))/6 + (255*diff_val)/2)/diff_val;
        diff_blue = ((255*(max_val-blue ))/6 + (255*diff_val)/2)/diff_val;
            
        if      (red  ==max_val) hue=diff_blue-diff_green;
        else if (green==max_val) hue=255/3+diff_red-diff_blue;
        else if (blue ==max_val) hue=510/3+diff_green-diff_red;
        
        if (hue<0)   hue+=255;
        if (hue>255) hue-=255;
    }
    
    //Log("hue: %d",hue);
    //Log("sat: %d",sat);
    //Log("lightness: %d",lightness);
    
    return (RGB(hue,sat,lightness));
}

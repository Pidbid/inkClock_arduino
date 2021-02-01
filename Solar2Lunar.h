/**************************************************************************************************************************
  ũ���ͽ�����ѯ
  ���ߣ�Kinneng  �黨ʹ��65686789
**************************************************************************************************************************/

#define uchar unsigned char
#define uint unsigned int

class Solar2Lunar 
{
    private :
    static uchar GetMoonDay(uchar month_p,unsigned short table_addr);
    void s2l(uint SolarYear,uchar SolarMonth,uchar SolarDay);
    uint _cror_(uint val,uint n);
    uchar Lunar_Month,Lunar_Day;
    uint Lunar_Year;
    bool flagLeapMonth;
    
   
    public :
    uchar SolarTerms(uint SolarYear,uchar SolarMonth,uchar SolarDay);//输出节气编号
    uchar LunarYear(uint SolarYear,uchar SolarMonth,uchar SolarDay); //输出干支记年，0表示甲子年，1为乙丑年，依此类推
    uchar LunarMonth(uint SolarYear,uchar SolarMonth,uchar SolarDay);
    uchar LunarDay(uint SolarYear,uchar SolarMonth,uchar SolarDay);
    bool isLeapMonth(uint SolarYear,uchar SolarMonth,uchar SolarDay);//输出所在月份是否为闰月，闰月为1
};

#include "Solar2Lunar.h"
#include <avr/pgmspace.h>

#define uchar unsigned char
#define uint unsigned int

/**************************************************************************************************************************
  查表法获得2016年到2050年之间的农历的年月日
  作者：Kinneng  麻花痛：65686789
  修改by南北大街东西走
**************************************************************************************************************************/

//数据表
//农历年数据的格式：闰月4位，大小月13位，春节所在公历月2位，春节所在公历日5位。
//一个农历年占3字节，此表从春节位于2019年的农历己亥年开始
const uchar LunarDataList[] PROGMEM =
    {
        0x0A,
        0x93,
        0x45,
        0x47,
        0x4A,
        0xB9,
        0x06,
        0xAA,
        0x4C,
        0x0A,
        0xD5,
        0x41,
        0x24,
        0xDA,
        0xB6,
        0x04,
        0xB6,
        0x4A,
        0x69,
        0x57,
        0x3D,
        0x0A,
        0x4E,
        0x51,
        0x0D,
        0x26,
        0x46,
        0x5E,
        0x93,
        0x3A,
        0x0D,
        0x53,
        0x4D,
        0x05,
        0xAA,
        0x43,
        0x36,
        0xB5,
        0x37,
        0x09,
        0x6D,
        0x4B,
        0xB4,
        0xAE,
        0xBF,
        0x04,
        0xAD,
        0x53,
        0x0A,
        0x4D,
        0x48,
        0x6D,
        0x25,
        0xBC,
        0x0D,
        0x25,
        0x4F,
        0x0D,
        0x52,
        0x44,
        0x5D,
        0xAA,
        0x38,
        0x0B,
        0x5A,
        0x4C,
        0x05,
        0x6D,
        0x41,
        0x24,
        0xAD,
        0xB6,
        0x04,
        0x9B,
        0x4A,
        0x7A,
        0x4B,
        0xBE,
        0x0A,
        0x4B,
        0x51,
        0x0A,
        0xA5,
        0x46,
        0x5B,
        0x52,
        0xBA,
        0x06,
        0xD2,
        0x4E,
        0x0A,
        0xDA,
        0x42,
        0x35,
        0x5B,
        0x37,
};

//月份天数表
const uchar day_code1[9] PROGMEM =
    {
        0x0, 0x1f, 0x3b, 0x5a, 0x78, 0x97, 0xb5, 0xd4, 0xf3};

//读取数据表中农历月的大月或小月,如果该月为大返回1,为小返回0
uchar Solar2Lunar ::GetMoonDay(uchar month_p, unsigned short table_addr)
{
    switch (month_p)
    {
    case 1:
        if ((pgm_read_byte(&LunarDataList[table_addr]) & 0x08) == 0)
            return (0);
        else
            return (1);
    case 2:
        if ((pgm_read_byte(&LunarDataList[table_addr]) & 0x04) == 0)
            return (0);
        else
            return (1);
    case 3:
        if ((pgm_read_byte(&LunarDataList[table_addr]) & 0x02) == 0)
            return (0);
        else
            return (1);
    case 4:
        if ((pgm_read_byte(&LunarDataList[table_addr]) & 0x01) == 0)
            return (0);
        else
            return (1);
    case 5:
        if ((pgm_read_byte(&LunarDataList[table_addr + 1]) & 0x80) == 0)
            return (0);
        else
            return (1);
    case 6:
        if ((pgm_read_byte(&LunarDataList[table_addr + 1]) & 0x40) == 0)
            return (0);
        else
            return (1);
    case 7:
        if ((pgm_read_byte(&LunarDataList[table_addr + 1]) & 0x20) == 0)
            return (0);
        else
            return (1);
    case 8:
        if ((pgm_read_byte(&LunarDataList[table_addr + 1]) & 0x10) == 0)
            return (0);
        else
            return (1);
    case 9:
        if ((pgm_read_byte(&LunarDataList[table_addr + 1]) & 0x08) == 0)
            return (0);
        else
            return (1);
    case 10:
        if ((pgm_read_byte(&LunarDataList[table_addr + 1]) & 0x04) == 0)
            return (0);
        else
            return (1);
    case 11:
        if ((pgm_read_byte(&LunarDataList[table_addr + 1]) & 0x02) == 0)
            return (0);
        else
            return (1);
    case 12:
        if ((pgm_read_byte(&LunarDataList[table_addr + 1]) & 0x01) == 0)
            return (0);
        else
            return (1);
    case 13:
        if ((pgm_read_byte(&LunarDataList[table_addr + 2]) & 0x80) == 0)
            return (0);
        else
            return (1);
    }
    return (0);
}

uint Solar2Lunar ::_cror_(uint val, uint n)
{
    n = n % 8;
    uint buf = val << (8 - n);
    val = val >> n;
    val |= buf;
    return val;
}

//公历转农历函数
void Solar2Lunar ::s2l(uint SolarYear, uchar SolarMonth, uchar SolarDay)
{
    uchar temp1, temp2, temp3, month_p;
    uint temp4, table_addr;
    uchar flag2, flag_y;
    flagLeapMonth = false;
    if ((SolarYear >= 2020) && (SolarYear <= 2050))
    {
        table_addr = (SolarYear - 2019) * 0x3;
        //取年数据的最后一个字节 例如 0x53 01010011
        // 1100000 & 01010011 = 01000000
        // 右移5位 00000010 0x02 即2月份春节 农历1月1日
        // 0011111 & 01010011 = 00010011 0x13 即19号春节
        //取当年春节所在的公历月份
        temp1 = pgm_read_byte(&LunarDataList[table_addr + 2]) & 0x60;
        temp1 = _cror_(temp1, 5);
        //取当年春节所在的公历日
        temp2 = pgm_read_byte(&LunarDataList[table_addr + 2]) & 0x1f;
        //取当年春节所在的公历日完成
        //计算当年春年离当年元旦的天数,春节只会在公历1月或2月
        if (temp1 == 0x1)
            temp3 = temp2 - 1;
        else
            temp3 = temp2 + 0x1f - 1;
        // 计算当年春年离当年元旦的天数完成
        //计算公历日离当年元旦的天数
        if (SolarMonth < 10)
            temp4 = pgm_read_byte(&day_code1[SolarMonth - 1]) + SolarDay - 1;
        else if (SolarMonth == 10)
            temp4 = 0x111 + SolarDay - 1;
        else if (SolarMonth == 11)
            temp4 = 0x130 + SolarDay - 1;
        else if (SolarMonth == 12)
            //e后面要留空格
            temp4 = 0x14e + SolarDay - 1;
        if ((SolarMonth > 0x2) && (SolarYear % 0x4 == 0) && ((SolarYear != 1900) && (SolarYear != 2100)))
            temp4 += 1;
        //如果公历月大于2月并且该年的2月为闰月,天数加1
        //计算公历日离当年元旦的天数完成
        //判断公历日在春节前还是春节后
        if (temp4 >= temp3)
        {
            temp4 -= temp3;
            SolarMonth = 0x1;
            month_p = 0x1;
            //month_p为月份指向,公历日在春节前或就是春节当日month_p指向首月
            flag2 = GetMoonDay(month_p, table_addr);
            //检查该农历月为大小还是小月,大月返回1,小月返回0
            flag_y = 0;
            if (flag2 == 0)
                temp1 = 0x1d;
            //小月29天
            else
                temp1 = 0x1e;
            //大月30天
            temp2 = pgm_read_byte(&LunarDataList[table_addr]) & 0xf0;
            temp2 = _cror_(temp2, 4);
            //从数据表中取该年的闰月月份,如为0则该年无闰月
            while (temp4 >= temp1)
            {
                temp4 -= temp1;
                month_p += 1;
                //                flagLeapMonth=flagLeapMonth2;
                if (SolarMonth == temp2) //当行进到闰月或其前一个月时执行
                {
                    flag_y = ~flag_y;
                    if (flag_y == 0)
                    {
                        SolarMonth += 1;
                        flagLeapMonth = false;
                    }
                    else
                        flagLeapMonth = true;
                }
                else
                {
                    SolarMonth += 1;
                    //		              flagLeapMonth=false;
                }
                flag2 = GetMoonDay(month_p, table_addr);
                if (flag2 == 0)
                    temp1 = 0x1d;
                else
                    temp1 = 0x1e;
            }
            SolarDay = temp4 + 1;
        }
        else
        {
            temp3 -= temp4; //现在temp3为这一天到下个春节的距离
            SolarYear -= 1;
            table_addr -= 0x3; //这一天位于前一个农历年
            SolarMonth = 0xc;  //从这一天所在农历年的腊月开始
            temp2 = pgm_read_byte(&LunarDataList[table_addr]) & 0xf0;
            temp2 = _cror_(temp2, 4); //取这一天所在农历年的闰月
            if (temp2 == 0)
                month_p = 0xc; //不存在闰月则指向第十二个农历月
            else
                month_p = 0xd; //存在闰月则指向第十三个农历月
            flag_y = 0;
            flag2 = GetMoonDay(month_p, table_addr);
            if (flag2 == 0)
                temp1 = 0x1d;
            else
                temp1 = 0x1e;
            while (temp3 > temp1)
            {
                temp3 -= temp1;
                month_p -= 1;
                if (flag_y == 0)
                    SolarMonth -= 1;
                if (SolarMonth == temp2)
                {
                    flag_y = ~flag_y;
                    flagLeapMonth = true;
                }
                if (flag_y == 0)
                    flagLeapMonth = false;
                flag2 = GetMoonDay(month_p, table_addr);
                if (flag2 == 0)
                    temp1 = 0x1d;
                else
                    temp1 = 0x1e;
            }
            SolarDay = temp1 - temp3 + 1;
            if (SolarMonth == 12 && temp2 == 12)
                flagLeapMonth = true;
        }
        Lunar_Year = SolarYear;
        Lunar_Month = SolarMonth;
        Lunar_Day = SolarDay;
    }
    else
    {
        Lunar_Year = 0;
        Lunar_Month = 0;
        Lunar_Day = 0;
    }
}

uchar Solar2Lunar ::LunarYear(uint SolarYear, uchar SolarMon, uchar SolarDay)
{
    s2l(SolarYear, SolarMon, SolarDay);
    return ((Lunar_Year - 1984) % 60);
}

uchar Solar2Lunar ::LunarMonth(uint SolarYear, uchar SolarMon, uchar SolarDay)
{
    s2l(SolarYear, SolarMon, SolarDay);
    return (Lunar_Month);
}

uchar Solar2Lunar ::LunarDay(uint SolarYear, uchar SolarMon, uchar SolarDay)
{
    s2l(SolarYear, SolarMon, SolarDay);
    return (Lunar_Day);
}

bool Solar2Lunar ::isLeapMonth(uint SolarYear, uchar SolarMon, uchar SolarDay)
{
    s2l(SolarYear, SolarMon, SolarDay);
    return (flagLeapMonth);
}

/**************************************************************************************************************************
  ������2020�굽2050��֮��Ľ�����ź���
  ���ߣ�Kinneng  �黨ʹ��65686789
**************************************************************************************************************************/

// ����������Ǵӳ������ת�������ģ�������Դ�������˵���Ʒ�����򾭹���֤��������֤���ݵ�׼ȷ�ԡ�
// ����������Ҫ28���ֽڣ�֮��ÿһ������1��4���ֽڣ���Ҫ��ʱ��������24���ֽڣ��Ƿǳ�����ķ����ˡ�

//���ݱ�
const uchar SolarTerms_List[] PROGMEM =
    {
        0x05,
        0x13,
        0x03,
        0x12,
        0x05,
        0x14,
        0x04,
        0x13,
        0x05,
        0x14,
        0x05,
        0x14,
        0x06,
        0x16,
        0x07,
        0x16,
        0x07,
        0x16,
        0x07,
        0x17,
        0x07,
        0x15,
        0x06,
        0x15,
        0x0F,
        0x03,
        0x08,
        0x0B,
        0x0E,
        0x03,
        0x07,
        0x0A,
        0x0E,
        0x03,
        0x07,
        0x0A,
        0x0E,
        0x03,
        0x07,
        0x09,
        0x0E,
        0x03,
        0x07,
        0x09,
        0x0E,
        0x02,
        0x06,
        0x09,
        0x0D,
        0x01,
        0x05,
        0x09,
        0x0C,
        0x00,
        0x04,
        0x00,
        0x10,
        0x26,
        0x40,
        0x19,
        0x26,
        0x41,
        0x19,
        0x26,
        0x41,
        0x59,
        0x66,
        0x41,
        0x59,
        0x67,
        0x61,
        0x59,
        0x67,
        0x61,
        0x5D,
        0x67,
        0x63,
        0x5D,
        0x67,
        0x73,
        0x7D,
        0x67,
        0x7F,
        0x7D,
        0xE7,
        0x7F,
        0xFF,
        0xE7,
        0x7F,
        0xFF,
        0xFF,
        0xF0,
        0x00,
        0x00,
        0xF0,
        0x10,
        0x04,
        0xF0,
        0x10,
        0x24,
        0xF0,
        0x10,
        0x26,
};

uchar Solar2Lunar ::SolarTerms(uint SolarYear, uchar SolarMon, uchar SolarDay)
{
    if (SolarYear < 2020)
        return (0);
    else if (SolarYear > 2050)
        return (0);
    else
    {
        uchar base, temp, id, offset;
        id = (SolarMon - 1) << 1;
        if (SolarDay > 15)
            id++;
        offset = id;
        offset = id;
        base = pgm_read_byte(&SolarTerms_List[offset]);
        temp = pgm_read_byte(&SolarTerms_List[pgm_read_byte(&SolarTerms_List[SolarYear - 1996]) * 3 + (SolarMon - 1) / 4 + 55]);
        offset = id;
        if (offset > 7)
            offset = offset - 8;
        if (offset > 7)
            offset = offset - 8;
        temp = (temp << offset) & 0x80;
        if (temp == 128)
            base = base + 1;
        if (base == SolarDay)
            return (id + 1);
        else
            return (0);
    }
}

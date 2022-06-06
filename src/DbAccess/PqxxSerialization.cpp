#include "Basis/DbAccess/PqxxSerialization.hpp"

namespace NTPro::Ecn::DbAccess
{

DbFieldsConverter::DbFieldsConverter(
    const DbAccess::PqxxReader::TRow& aRow,
    const DbFieldsGetter& aFields)
    : Row(aRow)
    , Fields(aFields)
{}

Basis::DateTime DbFieldsConverter::ParseDateTime(const std::string& aDbValue)
{
    int year{}, month{}, day{}, hour{}, min{}, sec{}, msec{}, usec{};
    std::vector<std::string> dateAndTime, yyyyMmDd, timeAndMs, hhMmSs;
                
    boost::split(dateAndTime, aDbValue, boost::is_any_of(" "));
    if (dateAndTime.empty() || dateAndTime.size() > 2)
    {
        throw std::invalid_argument(aDbValue);
    }
                
    boost::split(yyyyMmDd, dateAndTime[0], boost::is_any_of("-"));
    if (yyyyMmDd.size() != 3)
    {
        throw std::invalid_argument(aDbValue);
    }
                
    year = std::stoi(yyyyMmDd[0]);
    month = std::stoi(yyyyMmDd[1]);
    day = std::stoi(yyyyMmDd[2]);
                
    if (dateAndTime.size() == 2)
    {
        boost::split(timeAndMs, dateAndTime[1], boost::is_any_of("."));
        if (timeAndMs.empty() || timeAndMs.size() > 2)
        {
            throw std::invalid_argument(aDbValue);
        }

        boost::split(hhMmSs, timeAndMs[0], boost::is_any_of(":"));
        if (hhMmSs.size() != 3)
        {
            throw std::invalid_argument(aDbValue);
        }

        hour = std::stoi(hhMmSs[0]);
        min = std::stoi(hhMmSs[1]);
        sec = std::stoi(hhMmSs[2]);

        if (timeAndMs.size() == 2)
        {
            msec = std::stoi(timeAndMs[1]);
            if (msec > 999999)
            {
                throw std::invalid_argument(aDbValue);
            }
            if (msec > 999)
            {
                usec = msec % 1000;
                msec /= 1000;
            }
        }
    }

    return Basis::DateTime { year, month, day, hour, min, sec, msec, usec };
}
    
}

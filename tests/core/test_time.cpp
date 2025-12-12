/********************************************************************************
* @Author : hexne
* @Date   : 2025/12/11 22:39:30
********************************************************************************/

import modforge.time;
import std;



int test_time() {

    auto begin_utc_time = Time();
    auto begin_cst_time = LocalTime();
    // std::cout << begin_utc_time << std::endl;
    // std::cout << begin_cst_time << std::endl;

    auto cur_utc_time = Time(std::chrono::system_clock::now());
    auto cur_cst_time = LocalTime(std::chrono::system_clock::now());
    // std::cout << cur_utc_time << std::endl;
    // std::cout << cur_cst_time << std::endl;

    auto now_utc_time = Time::now();
    auto now_cst_time = LocalTime::now();
    // std::cout << now_utc_time << std::endl;
    // std::cout << now_cst_time << std::endl;

    now_utc_time.to_now();
    now_cst_time.to_now();
    // std::cout << now_utc_time << std::endl;
    // std::cout << now_cst_time << std::endl;

    auto utc_time = Time(2025, 1, 1, 0, 0, 0);
    if (utc_time != Time::from_date(2025, 1, 1))
        return __LINE__;

    auto cst_time = LocalTime(2025, 1, 1, 0, 0, 0);
    if (cst_time != LocalTime::from_date(2025, 1, 1))
        return __LINE__;

    if (Time::from_date("2025-1-1") != Time::from_date(2025, 1 ,1)
        or LocalTime::from_date("2025-1-1") != LocalTime::from_date(2025, 1 ,1))
        return __LINE__;

    if (Time::from_date("2025/1/1") != Time::from_date(2025, 1 ,1)
        or LocalTime::from_date("2025/1/1") != LocalTime::from_date(2025, 1 ,1))

    if (Time::from_clock("0:0:0") != Time::from_clock(0, 0, 0)
        or LocalTime::from_clock("0:0:0") != LocalTime::from_clock(0, 0, 0))
        return __LINE__;

    if (Time("2025/1/1 0:0:0") != Time(2025, 1, 1, 0, 0, 0)
        or LocalTime("2025/1/1 0:0:0") != LocalTime(2025, 1, 1, 0, 0, 0))
        return __LINE__;

    utc_time = Time("2025/1/1 1:2:3");
    if (utc_time.get<std::chrono::year>() != 2025
        or utc_time.get<std::chrono::month>() != 1
        or utc_time.get<std::chrono::day>() != 1
        or utc_time.get<std::chrono::hours>() != 1
        or utc_time.get<std::chrono::minutes>() != 2
        or utc_time.get<std::chrono::seconds>() != 3)
        return __LINE__;

    cst_time = LocalTime("2025/1/1 1:2:3");
    if (cst_time.get<std::chrono::year>() != 2025
        or cst_time.get<std::chrono::month>() != 1
        or cst_time.get<std::chrono::day>() != 1
        or cst_time.get<std::chrono::hours>() != 1
        or cst_time.get<std::chrono::minutes>() != 2
        or cst_time.get<std::chrono::seconds>() != 3)
        return __LINE__;

    utc_time = Time(std::chrono::minutes{10});
    cst_time = LocalTime(std::chrono::minutes{10});
    if (static_cast<std::chrono::seconds>(utc_time) != std::chrono::seconds{10 * 60}
        or static_cast<std::chrono::seconds>(cst_time) != std::chrono::seconds{10 * 60})
        return __LINE__;

    utc_time -= std::chrono::seconds{30};
    cst_time -= std::chrono::seconds{30};
    if (static_cast<std::chrono::seconds>(utc_time) != std::chrono::seconds{10 * 60 - 30}
        or static_cast<std::chrono::seconds>(cst_time) != std::chrono::seconds{10 * 60 - 30})
        return __LINE__;

    utc_time += std::chrono::seconds{30};
    cst_time += std::chrono::seconds{30};
    if (static_cast<std::chrono::seconds>(utc_time) != std::chrono::seconds{10 * 60}
        or static_cast<std::chrono::seconds>(cst_time) != std::chrono::seconds{10 * 60})
        return __LINE__;

    if (utc_time.count<std::chrono::minutes>() != 10
        or cst_time.count<std::chrono::minutes>() != 10)
        return __LINE__;

    if (cur_utc_time.count<std::chrono::years>() != 2024
        or cur_cst_time.count<std::chrono::years>() != 2024)
        return __LINE__;


    auto res1 = (cur_utc_time - utc_time).count<std::chrono::years>();
    auto res2 = (cur_cst_time - cst_time).count<std::chrono::years>();

    std::cout << cur_utc_time << " - " << utc_time << " = " << (cur_utc_time - utc_time) << " , count is " << res1 << std::endl;
    std::cout << cur_cst_time << " - " << cst_time << " = " << (cur_cst_time - cst_time) << " , count is " << res2 << std::endl;
    // std::cout << res1 << std::endl;
    // std::cout << res2 << std::endl;




    return 0;
}

-- My_PID_Motor v2.3.1 timing aggregate-field smoke test
-- LLCom serial settings: 115200, 8N1, text mode, CRLF
-- This test validates only the newly added aggregate TIMING_STATS fields.
-- It does not rerun M0-M3 and does not decide PASS/FAIL automatically.

local COMMAND_WAIT_MS = 500
local STATUS_WAIT_MS = 1000
local AGGREGATE_WAIT_MS = 1000
local SAMPLE_COUNT = 3
local LOG_PATH = "F:/hal_stm32_project/My_PID_Motor/logs/v2.3_timing_aggregate_smoke_raw_" ..
    os.date("%Y%m%d_%H%M%S") .. ".txt"

local raw_log, log_error = io.open(LOG_PATH, "wb")
if raw_log == nil then
    log.info("LOG_OPEN_FAIL", LOG_PATH, log_error)
    error("Cannot open test log: " .. tostring(log_error))
end

raw_log:write("LLCom v2.3.1 timing aggregate-field smoke test\r\n")
raw_log:write("firmware_branch=v2.3.1-timing-observability\r\n")
raw_log:write("firmware_commit=3aa18c2\r\n")
raw_log:write("working_tree=uncommitted v2.3 timing changes; verify before release\r\n")
raw_log:write("serial=115200,8N1,CRLF\r\n")
raw_log:write("script_log=", LOG_PATH, "\r\n\r\n")
raw_log:flush()

local function writeTrace(kind, data)
    if raw_log == nil then
        return
    end
    raw_log:write("[", os.date("%Y-%m-%d %H:%M:%S"), "] ", kind, " ")
    raw_log:write(data)
    if string.sub(data, -1) ~= "\n" then
        raw_log:write("\r\n")
    end
    raw_log:flush()
end

local function writeMark(kind, data)
    log.info(kind, data)
    writeTrace(kind, data)
end

local function closeLog()
    if raw_log ~= nil then
        raw_log:flush()
        raw_log:close()
        raw_log = nil
    end
end

uartReceive = function(data)
    log.info("RX", data)
    writeTrace("RX", data)
end

local function sendCommand(command, wait_ms)
    log.info("TX", command)
    writeTrace("TX", command .. "\r\n")
    if not apiSend("uart", command .. "\r\n") then
        writeMark("TX_FAIL", command)
        return false
    end
    sys.wait(wait_ms or COMMAND_WAIT_MS)
    return true
end

local function requireCommand(command, wait_ms)
    if sendCommand(command, wait_ms) then
        return true
    end
    writeMark("SCRIPT_ABORT", "Check the serial connection and save the log.")
    closeLog()
    return false
end

sys.taskInit(function()
    writeMark("TEST_START", "Timing aggregate-field smoke")
    writeMark("PURPOSE", "Validate sample_count, min/max values and max_abs_jitter_us output/update.")
    writeMark("EXPECT_FIELDS", "sample_count,period_us,period_min_us,period_max_us,max_abs_jitter_us,exec_us,exec_min_us,exec_max_us,tick_exec_us,tick_exec_min_us,tick_exec_max_us,timeout_count")

    if not requireCommand("stop") then return end
    if not requireCommand("set target uart") then return end
    if not requireCommand("t=600") then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end

    for index = 1, SAMPLE_COUNT do
        writeMark("SAMPLE", tostring(index))
        if not requireCommand("timing stats", COMMAND_WAIT_MS) then return end
        if index < SAMPLE_COUNT then
            sys.wait(AGGREGATE_WAIT_MS)
        end
    end

    if not requireCommand("comm stats", STATUS_WAIT_MS) then return end

    writeMark("CHECK_1", "sample_count should be greater than zero.")
    writeMark("CHECK_2", "sample_count should increase between samples 1, 2 and 3.")
    writeMark("CHECK_3", "period_min_us <= period_us <= period_max_us.")
    writeMark("CHECK_4", "exec_min_us <= exec_us <= exec_max_us.")
    writeMark("CHECK_5", "tick_exec_min_us <= tick_exec_us <= tick_exec_max_us.")
    writeMark("CHECK_6", "max_abs_jitter_us >= abs(period_us - 10000).")
    writeMark("CHECK_7", "No parse error, queue error, pool failure or TX error.")
    writeMark("TEST_DONE", "Review the three TIMING_STATS lines and COMM_STATS manually.")
    closeLog()
end)

-- My_PID_Motor v2.3.1 M0 timing-observability smoke test
-- LLCom serial settings: 115200, 8N1, text mode, CRLF
-- The script creates a timestamped TX/RX log under the project logs directory.
-- This script does not decide PASS/FAIL automatically; review TIMING_STATS lines.

local COMMAND_WAIT_MS = 500
local STATUS_WAIT_MS = 1000
local SAMPLE_COUNT = 3
local LOG_PATH = "F:/hal_stm32_project/My_PID_Motor/logs/v2.3_m0_timing_smoke_raw_" ..
    os.date("%Y%m%d_%H%M%S") .. ".txt"

local raw_log, log_error = io.open(LOG_PATH, "wb")
if raw_log == nil then
    log.info("LOG_OPEN_FAIL", LOG_PATH, log_error)
    error("Cannot open test log: " .. tostring(log_error))
end

raw_log:write("LLCom v2.3.1 M0 timing-observability smoke test\r\n")
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
    writeMark("TEST_START", "M0 timing-observability smoke")
    writeMark("PURPOSE", "Verify timing stats command, snapshot, queue and UART output chain.")

    if not requireCommand("stop") then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end

    for index = 1, SAMPLE_COUNT do
        writeMark("SAMPLE", tostring(index))
        if not requireCommand("timing stats", COMMAND_WAIT_MS) then return end
    end

    if not requireCommand("comm stats", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "Three complete TIMING_STATS lines with numeric fields; no parse error.")
    writeMark("TEST_DONE", "Review TIMING_STATS and COMM_STATS in the raw log.")
    closeLog()
end)

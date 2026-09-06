-- My_PID_Motor v2.3.1 M3 command-pressure timing test
-- LLCom serial settings: 115200, 8N1, text mode, CRLF
-- Safety: keep the motor stopped; this test stresses command/control queues only.
-- The script creates a timestamped TX/RX log under the project logs directory.
-- This script does not decide PASS/FAIL automatically; review COMM_STATS and timing samples.

local COMMAND_WAIT_MS = 250
local STATUS_WAIT_MS = 1000
local BURST_COUNT = 50
local BURST_INTERVAL_MS = 20
local DRAIN_WAIT_MS = 5000
local LOG_PATH = "F:/hal_stm32_project/My_PID_Motor/logs/v2.3_m3_timing_command_pressure_raw_" ..
    os.date("%Y%m%d_%H%M%S") .. ".txt"

local raw_log, log_error = io.open(LOG_PATH, "wb")
if raw_log == nil then
    log.info("LOG_OPEN_FAIL", LOG_PATH, log_error)
    error("Cannot open test log: " .. tostring(log_error))
end

raw_log:write("LLCom v2.3.1 M3 command-pressure timing test\r\n")
raw_log:write("firmware_branch=v2.3.1-timing-observability\r\n")
raw_log:write("firmware_commit=3aa18c2\r\n")
raw_log:write("working_tree=uncommitted v2.3 timing changes; verify before release\r\n")
raw_log:write("serial=115200,8N1,CRLF\r\n")
raw_log:write("motor_state=stopped\r\n")
raw_log:write("burst_count=", tostring(BURST_COUNT), "\r\n")
raw_log:write("burst_interval_ms=", tostring(BURST_INTERVAL_MS), "\r\n")
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

local function abortWithStop(reason)
    writeMark("SCRIPT_ABORT", reason)
    apiSend("uart", "stop\r\n")
    writeTrace("TX", "stop\r\n")
    closeLog()
end

local function requireCommand(command, wait_ms)
    if sendCommand(command, wait_ms) then
        return true
    end
    abortWithStop("Send failed; stop was attempted.")
    return false
end

local function sendBurstCommand(command)
    log.info("TX_BURST", command)
    writeTrace("TX_BURST", command .. "\r\n")
    if not apiSend("uart", command .. "\r\n") then
        writeMark("TX_FAIL", command)
        return false
    end
    sys.wait(BURST_INTERVAL_MS)
    return true
end

sys.taskInit(function()
    writeMark("TEST_START", "M3 command-pressure timing")
    writeMark("PURPOSE", "Stress command parsing and queues while the motor remains stopped.")

    if not requireCommand("stop") then return end
    if not requireCommand("set target uart") then return end
    if not requireCommand("t=600") then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    if not requireCommand("timing stats", COMMAND_WAIT_MS) then return end
    if not requireCommand("comm stats", STATUS_WAIT_MS) then return end

    writeMark("CASE", "alternating target burst")
    for index = 1, BURST_COUNT do
        local command = (index % 2 == 1) and "t=500" or "t=600"
        writeMark("BURST_INDEX", tostring(index))
        if not sendBurstCommand(command) then
            abortWithStop("Burst send failed; stop was attempted.")
            return
        end
    end

    writeMark("DRAIN", "Wait for queued command responses and TX completion.")
    sys.wait(DRAIN_WAIT_MS)
    if not requireCommand("timing stats", COMMAND_WAIT_MS) then return end
    if not requireCommand("comm stats", STATUS_WAIT_MS) then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "Motor remains IDLE/PWM=0; inspect queue-full, pool-fail, TX-error and timing counters.")

    if not requireCommand("stop", 700) then return end
    writeMark("TEST_DONE", "Review every response and final TIMING_STATS/COMM_STATS.")
    closeLog()
end)

-- My_PID_Motor v2.4 same-direction target-drop reproduction
-- LLCom serial settings: 115200, 8N1, text mode, CRLF
-- Purpose: reproduce and compare 800->500 target drop against direct start at 500.
-- This script records raw status lines; it does not decide PASS/FAIL automatically.
-- Safety: fix the motor, keep it unloaded, and keep power-off available.

local COMMAND_WAIT_MS = 500
local STATUS_WAIT_MS = 700
local SAMPLE_WAIT_MS = 50
local RUN_SETTLE_MS = 2000
local DROP_OBSERVE_MS = 2000
local DIRECT_OBSERVE_MS = 2000
local LOG_PATH = "F:/hal_stm32_project/My_PID_Motor/logs/v2.4_same_direction_target_drop_repro_raw_" ..
    os.date("%Y%m%d_%H%M%S") .. ".txt"

local raw_log, log_error = io.open(LOG_PATH, "wb")
if raw_log == nil then
    log.info("LOG_OPEN_FAIL", LOG_PATH, log_error)
    error("Cannot open test log: " .. tostring(log_error))
end

raw_log:write("LLCom v2.4 same-direction target-drop reproduction\r\n")
raw_log:write("firmware_branch=v2.4\r\n")
raw_log:write("firmware_commit=f49ecb5\r\n")
raw_log:write("baseline_pid=kp=0.05,ki=0,kd=0\r\n")
raw_log:write("serial=115200,8N1,CRLF\r\n")
raw_log:write("sample_interval_ms=", tostring(SAMPLE_WAIT_MS), "\r\n")
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
    local delay_ms = wait_ms or COMMAND_WAIT_MS
    if delay_ms <= 0 then
        delay_ms = 1
    end
    sys.wait(delay_ms)
    return true
end

local function stopAndClose(reason)
    writeMark("SCRIPT_ABORT", reason)
    apiSend("uart", "stop\r\n")
    writeTrace("TX", "stop\r\n")
    closeLog()
end

local function requireCommand(command, wait_ms)
    if sendCommand(command, wait_ms) then
        return true
    end
    stopAndClose("Send failed; stop was attempted.")
    return false
end

local function observeStatusPhase(name, duration_ms)
    writeMark("PHASE_START", name .. ",duration_ms=" .. tostring(duration_ms))
    local elapsed = 0
    while elapsed < duration_ms do
        if not sendCommand("status", 20) then
            stopAndClose("Status sampling failed; stop was attempted.")
            return false
        end
        sys.wait(SAMPLE_WAIT_MS)
        elapsed = elapsed + SAMPLE_WAIT_MS
    end
    writeMark("PHASE_END", name)
    return true
end

local function runTest()
    writeMark("TEST_START", "same-direction target-drop reproduction")
    writeMark("PURPOSE", "Compare 800->500 while running against direct start at 500.")
    writeMark("SUSPECT", "After same-direction target drop: actual may become 0 while PWM remains near 140 and State remains RUN.")
    writeMark("SAFETY", "Stop and remove power immediately if noise, runaway, heat or fault appears.")

    if not requireCommand("stop") then return end
    if not requireCommand("set target uart") then return end
    if not requireCommand("kp=0.05") then return end
    if not requireCommand("ki=0") then return end
    if not requireCommand("kd=0") then return end

    writeMark("CASE", "A: run at 800 then drop to 500")
    if not requireCommand("t=800") then return end
    if not requireCommand("run=1", RUN_SETTLE_MS) then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "Before drop: State=RUN, target=800, actual should be nonzero.")
    if not requireCommand("t=500") then return end
    if not observeStatusPhase("DROP_800_TO_500", DROP_OBSERVE_MS) then return end
    if not requireCommand("stop", 700) then return end

    writeMark("CASE", "B: direct start at 500 control")
    if not requireCommand("t=500") then return end
    if not requireCommand("run=1", RUN_SETTLE_MS) then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "Direct start: State=RUN, target=500, actual should be nonzero or show startup behavior.")
    if not observeStatusPhase("DIRECT_START_500", DIRECT_OBSERVE_MS) then return end

    writeMark("CASE", "final stop")
    if not requireCommand("stop", 700) then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    if not requireCommand("get fault", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "Final state: enable=0, State=IDLE, PWM=0, fault=NONE.")
    writeMark("ANALYSIS", "Compare DROP_800_TO_500 with DIRECT_START_500; focus on actual, delta, PWM, State and fault.")
    writeMark("TEST_DONE", "Review raw status lines; no automatic PASS/FAIL was applied.")
    closeLog()
end

sys.taskInit(function()
    local ok, err = pcall(runTest)
    if not ok then
        stopAndClose("Lua runtime error: " .. tostring(err))
    end
end)

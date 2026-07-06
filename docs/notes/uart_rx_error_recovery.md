1.串口在接收过程中出现错误会进入ErrorCallback，在这里面选择直接重新启动接收可能会刚好碰上再次重启失败，这时候重启失败就无法再进入ErrorCallback了，所以进入ErrorCallback后置标志位s_rx_restart_pending，让他后面其他地方通过这个标志位重新接收，这样如果多次出错的话就可以不断进入ErrorCallback了。

2.但这样可能会出现这种情况：
HAL_UART_ErrorCallback() 触发了
    ↓
pending = 1
    ↓
但是 HAL 内部 RX 其实还没完全停，或者 ReceiveToIdle 还在运行（就是进入 ErrorCallback 后串口不一定会停止，有些可能需要等一会才会停止，甚至有的压根不会停止）
    ↓
Uart_ProcessRx() 尝试恢复
    ↓
发现 RxState 不是 READY（也就是说这个错误没有严重到需要重新启动）
    ↓
说明现在不适合重新启动，或者已经在接收
    ↓
此时强行调用 HAL_UARTEx_ReceiveToIdle_IT()，可能会返回 HAL_BUSY而不是HAL_OK，从而让代码走到s_rx_restart_fail_count++的位置，但实际上这个并不算错误。

为了避免这种情况就加一个    
if (huart1.RxState != HAL_UART_STATE_READY)
    {
        s_rx_restart_pending = 0U;
        return 1U;
    }
就可以避免那些“没有严重到需要重启串口的错误”也引起重启串口的恢复措施了


3.s_rx_restart_fail_count++;就是增加可诊断性，问题定位能力
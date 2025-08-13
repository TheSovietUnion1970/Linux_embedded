void can0_init(struct can_device_data *data) {
    u32 can_ctl = 0, if1cmd = 0, if1mctl = 0;

    can_ctl = CAN_CTL_INIT | CAN_CTL_CCE | CAN_CTL_DAR; // CAN_CTL_DAR is used to disable auto retransmission
    iowrite32(can_ctl, data->base + CAN_CTL); // enter init mode, access to registers
    //while((ioread32(data->base + CAN_CTL)&CAN_CTL_INIT) == CAN_CTL_INIT); // wait init = 1;
    wait_register_update(data, CAN_CTL, CAN_CTL_INIT_OFFSET, 1, MS_DELAY, "INIT mode");

    // Bit timing values into BTR, output bitrate is 500KHz
    can0_bit_timing(data, clk_get_rate(data->clk), 500000);

    // clear init, CCE
    can_ctl &=~ (CAN_CTL_INIT | CAN_CTL_CCE);
    iowrite32(can_ctl, data->base + CAN_CTL); // enter init mode, access to registers
    //while((ioread32(data->base + CAN_CTL)&CAN_CTL_INIT) != CAN_CTL_INIT); // wait init = 0;
    wait_register_update(data, CAN_CTL, CAN_CTL_INIT_OFFSET, 0, MS_DELAY, "Normal mode");
    printk("ctl[0] = 0x%x\n", ioread32(data->base + CAN_CTL));

    if1mctl = ioread32(data->base + CAN_IF1MCTL);
    if1mctl &=~ (1u << 8); // :CAN_IFxMCTL_TxRqst message object is not waiting for a transmission
    iowrite32(if1mctl, data->base + CAN_IF1MCTL); 
    iowrite32(if1cmd, data->base + CAN_IF1CMD); // Apply the update
}

void can0_msg_obj_Data_Frames(struct can_device_data *data){
    u32 if1cmd = 0, if1msk = 0, if1mctl = 0, if1arb = 0, if1data = 0;

    // Transfer a complete message structure into a message object.
    iowrite32(0, data->base + CAN_IF1ARB); // reset arb registers
    if1arb = (1u << 31)&(CAN_IFxARB_MsgVal); // The message object is to be used by the message handler.
    if1arb &=~ (1U << 30); // CAN_IFxARB_Xtd: no Extended Identifier
    if1arb |= (1U << 29); // CAN_IFxARB_Dir: transmit
    if1arb |= (ID_CAN0 << 18)&(CAN_IFxARB_ID); // 11-bit ID for [28:18]
    iowrite32(if1arb, data->base + CAN_IF1ARB); 

    // Transfer the data bytes of a message into a message object 
    if1data |= TX[0]&(CAN_IFxDATA_0);
    if1data |= (TX[1] << 8)&(CAN_IFxDATA_1);
    if1data |= (TX[2] << 16)&(CAN_IFxDATA_2);
    iowrite32(if1data, data->base + CAN_IF1DATA); 

    // config msg control,  set TxRqst 
    if1mctl &=~ (1u << 12); // CAN_IFxMCTL_UMask: mask ignored
    if1mctl |= (1u << 7); // CAN_IFxMCTL_EoB: a single msg obj
    if1mctl &=~ (1u << 15); // CAN_IFxMCTL_NewDat: msg_hler/CPU write no new data
    if1mctl |= (1u << 8); // :CAN_IFxMCTL_TxRqst message object is waiting for a transmission
    if1mctl |= (0x3); // DLC = 1 byte
    if1mctl |= (1u << 11); // TxIE
    iowrite32(if1mctl, data->base + CAN_IF1MCTL); 


    // ===== config cmd as the last config
    if1cmd = ioread32(data->base + CAN_IF1CMD);

    if1cmd = (1u << 21)&(CAN_IFxCMD_Arb); // Access arbitration bits
    if1cmd &=~ (1u << 22); // no use mask

    if1cmd |= (1u << 23); // CAN_IFxCMD_WR_RD: Write
    if1cmd &=~ (1u << 18); // TxRqst_NewDat will by handled by CAN_IFxMCTL_TxRqst or CAN_IFxMCTL_NewDat in CAN_IFxMCTL

    if1cmd |= (1u << 20); // access control bits: msg control bits is transfered FROM IF1 register set TO message object by message number (Bits [7:0]).
    if1cmd |= (1u << 17); // use DATA_A: The data bytes 0-3 will be   transfered FROM IF1 register set TO message object by message number (Bits [7:0]).

    iowrite32(if1cmd, data->base + CAN_IF1CMD);

}

static ssize_t can0_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct can_device_data *data = filp->private_data;
    u32 can_if1cmd = 0, if1mctl = 0;

    // Config message object
    can0_msg_obj_Data_Frames(data);

    can_if1cmd = ioread32(data->base + CAN_IF1CMD);
    can_if1cmd |= 1u << 0; // CAN_IFxCMD_msgnum = 1, trigger transfer
    iowrite32(can_if1cmd, data->base + CAN_IF1CMD);

    if ((ioread32(data->base + CAN_IF1CMD)&BIT(15)) == BIT(15)) printk("HEHEHE\n");

    // wait the hanler send msg object from IF1 registers to msg RAM
    wait_register_update(data, CAN_IF1CMD, CAN_IFxCMD_Busy_OFFSET, 0, MS_DELAY, "Busy bit");

    if1mctl = ioread32(data->base + CAN_IF1MCTL);
    if1mctl &=~ (1u << 8); // :CAN_IFxMCTL_TxRqst message object is not waiting for a transmission
    iowrite32(if1mctl, data->base + CAN_IF1MCTL); 
    iowrite32(can_if1cmd, data->base + CAN_IF1CMD);

    // Check IntPnd
    if ((ioread32(data->base + CAN_IF1MCTL)&BIT(13)) == BIT(13)) printk("cmd -> sent\n");

    dev_info(data->dev, "Done transmitting data frame \n");

    return count;
}

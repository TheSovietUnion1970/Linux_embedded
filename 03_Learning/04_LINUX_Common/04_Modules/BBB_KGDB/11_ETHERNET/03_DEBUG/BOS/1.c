static int davinci_mdio_probe(struct platform_device *pdev)
ret = of_mdiobus_register(data->bus, dev->of_node);
rc = __mdiobus_register(mdio, owner);
err = bus->reset(bus); -> data->bus->reset	= davinci_mdio_reset;
=> 
[ 1510.281879] [V] davinci_mdio_reset

===
static int davinci_mdio_probe(struct platform_device *pdev)
ret = of_mdiobus_register(data->bus, dev->of_node);
rc = of_mdiobus_register_phy(mdio, child, addr);
fwnode_mdiobus_register_phy(mdio, of_fwnode_handle(child), addr);
phy = get_phy_device(bus, addr, is_c45);
r = get_phy_c22_id(bus, addr, &phy_id);
=>
[ 1510.281879] davinci_mdio 4a101000.mdio: davinci mdio revision 1.6, bu
s freq 1000000
[ 1510.291980] [V] davinci_mdio_read, phy_reg = 2, ret = 0x7
[ 1510.297865] [V] davinci_mdio_read, phy_reg = 3, ret = 0xc0f1

===
phy_probe
phy_disable_interrupts(phydev);
phy_config_interrupt(phydev, PHY_INTERRUPT_DISABLED); -> phydev->drv->config_intr(phydev);
=>
[ 1258.706192] [V] phy_probe
[ 1258.708861] [V] phy_drv_supports_irq -> config_intr
[ 1258.725843] [V] phy_disable_interrupts
[ 1258.729645] [V] phy_config_interrupt, int = 0
[ 1258.734087] [V] smsc_phy_config_intr
[ 1258.737682] [V] davinci_mdio_write, phy_reg = 30, phy_data = 0x0
[ 1258.744617] [V] davinci_mdio_read, phy_reg = 29, ret = 0x90

===
err = phydrv->get_features(phydev); or err = genphy_read_abilities(phydev);
=>
[ 1258.750662] [V] genphy_read_abilities - val = 0x7809


================================================================================================
INIT_DELAYED_WORK(&dev->state_queue, phy_state_machine);
PHY_CABLETEST -> phy_abort_cable_test(phydev); -> err = phy_init_hw(phydev);

void phy_state_machine(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct phy_device *phydev =
			container_of(dwork, struct phy_device, state_queue);
	struct net_device *dev = phydev->attached_dev;
	bool needs_aneg = false, do_suspend = false;
	enum phy_state old_state;
	bool finished = false;
	int err = 0;

	mutex_lock(&phydev->lock);

	old_state = phydev->state;

	printk("[V] phydev->state = %d\n", phydev->state);
	switch (phydev->state) {
	case PHY_DOWN:
	case PHY_READY:
		break;
	case PHY_UP:
		needs_aneg = true;

		break;
	case PHY_NOLINK:
	case PHY_RUNNING:
		err = phy_check_link_status(phydev);
		break;
	case PHY_CABLETEST:
		err = phydev->drv->cable_test_get_status(phydev, &finished);
		if (err) {
			printk("[V] phy_abort_cable_test(phydev);\n");
			phy_abort_cable_test(phydev);
			netif_testing_off(dev);
			needs_aneg = true;
			phydev->state = PHY_UP;
			break;
		}

		if (finished) {
			ethnl_cable_test_finished(phydev);
			netif_testing_off(dev);
			needs_aneg = true;
			phydev->state = PHY_UP;
		}
		break;
	case PHY_HALTED:
		if (phydev->link) {
			phydev->link = 0;
			phy_link_down(phydev);
		}
		do_suspend = true;
		break;
	}

	mutex_unlock(&phydev->lock);

	if (needs_aneg)
		err = phy_start_aneg(phydev);
	else if (do_suspend)
		phy_suspend(phydev);

	if (err == -ENODEV)
		return;

	if (err < 0)
		phy_error(phydev);

	phy_process_state_change(phydev, old_state);

	/* Only re-schedule a PHY state machine change if we are polling the
	 * PHY, if PHY_MAC_INTERRUPT is set, then we will be moving
	 * between states from phy_mac_interrupt().
	 *
	 * In state PHY_HALTED the PHY gets suspended, so rescheduling the
	 * state machine would be pointless and possibly error prone when
	 * called from phy_disconnect() synchronously.
	 */
	mutex_lock(&phydev->lock);
	if (phy_polling_mode(phydev) && phy_is_started(phydev))
		phy_queue_state_machine(phydev, PHY_STATE_TIME);
	mutex_unlock(&phydev->lock);
}

===
INIT_DELAYED_WORK(&dev->state_queue, phy_state_machine);

mod_delayed_work(system_power_efficient_wq, &phydev->state_queue,
			 jiffies);
			 
	if (phy_polling_mode(phydev))
		phy_trigger_machine(phydev);
===
Add printk before mdiobus_write( in phy.c and phylink.c 
Add printk before ->config_intr in phy.c and phy_device.c 

=== Inspections:
phylink.c fwnode_mdio.c -> not shown in the stack
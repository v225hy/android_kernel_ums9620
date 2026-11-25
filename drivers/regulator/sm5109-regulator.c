// SPDX-License-Identifier: GPL-2.0-only
/*
 * CP3NV1 LCD bias/reset sequencer, using SM5109-compatible I2C writes.
 *
 * No voltage conversion is exposed: the available SM5109 feature sheet
 * does not specify the register encoding.  Replay stock U-Boot's bytes.
 * This is a board-specific supply, not a generic SM5109 regulator driver.
 */
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>

#define CP3NV1_BIAS_CODE 0x14

struct cp3nv1_bias {
	struct regmap *regmap;
	struct gpio_desc *enp;
	struct gpio_desc *enn;
	struct gpio_desc *reset;
};

static const struct regmap_config cp3nv1_bias_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x01,
	/* No cache: replay both writes after every power cycle. */
};

static int cp3nv1_bias_power_off(struct cp3nv1_bias *bias)
{
	int ret, first;

	first = gpiod_direction_output(bias->reset, 0);
	msleep(5);
	/* Reverse-order rail shutdown is experimental, not recovered U-Boot. */
	ret = gpiod_direction_output(bias->enn, 0);
	if (!first)
		first = ret;
	msleep(5);
	ret = gpiod_direction_output(bias->enp, 0);
	if (!first)
		first = ret;
	msleep(5);
	return first;
}

static int cp3nv1_bias_enable(struct regulator_dev *rdev)
{
	struct cp3nv1_bias *bias = rdev_get_drvdata(rdev);
	int ret, cleanup;

	ret = gpiod_direction_output(bias->enp, 1);
	if (ret)
		goto fail;
	msleep(10);
	ret = gpiod_direction_output(bias->enn, 1);
	if (ret)
		goto fail;
	msleep(20);
	ret = gpiod_direction_output(bias->reset, 1);
	if (ret)
		goto fail;
	msleep(5);
	ret = gpiod_direction_output(bias->reset, 0);
	if (ret)
		goto fail;
	msleep(5);
	ret = gpiod_direction_output(bias->reset, 1);
	if (ret)
		goto fail;
	msleep(20);

	/* Stock U-Boot performs an ID read between reset and these writes.
	 * The Linux supply cannot access the DSI host here; the caller must
	 * select this opt-in board node only for the matching panel.
	 */
	ret = regmap_write(bias->regmap, 0x00, CP3NV1_BIAS_CODE);
	if (ret)
		goto fail;
	ret = regmap_write(bias->regmap, 0x01, CP3NV1_BIAS_CODE);
	if (!ret)
		return 0;
fail:
	dev_err(&rdev->dev, "LCD power sequence failed: %d\n", ret);
	cleanup = cp3nv1_bias_power_off(bias);
	if (cleanup)
		dev_err(&rdev->dev, "LCD power rollback failed: %d\n", cleanup);
	return ret;
}

static int cp3nv1_bias_disable(struct regulator_dev *rdev)
{
	return cp3nv1_bias_power_off(rdev_get_drvdata(rdev));
}

static int cp3nv1_bias_is_enabled(struct regulator_dev *rdev)
{
	struct cp3nv1_bias *bias = rdev_get_drvdata(rdev);
	int enp, enn;

	enp = gpiod_get_value_cansleep(bias->enp);
	if (enp < 0)
		return enp;
	enn = gpiod_get_value_cansleep(bias->enn);
	if (enn < 0)
		return enn;
	return enp && enn;
}

static const struct regulator_ops cp3nv1_bias_ops = {
	.enable = cp3nv1_bias_enable,
	.disable = cp3nv1_bias_disable,
	.is_enabled = cp3nv1_bias_is_enabled,
};

static const struct regulator_desc cp3nv1_bias_desc = {
	.name = "cp3nv1-lcd-bias",
	.of_match = "lcd-bias",
	.regulators_node = "regulators",
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
	.ops = &cp3nv1_bias_ops,
};

static int cp3nv1_bias_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	struct regulator_config config = { .dev = &client->dev };
	struct regulator_dev *rdev;
	struct cp3nv1_bias *bias;

	bias = devm_kzalloc(&client->dev, sizeof(*bias), GFP_KERNEL);
	if (!bias)
		return -ENOMEM;
	bias->regmap = devm_regmap_init_i2c(client, &cp3nv1_bias_regmap);
	if (IS_ERR(bias->regmap))
		return PTR_ERR(bias->regmap);

	/* Do not glitch a panel left running by U-Boot when probing. */
	bias->enp = devm_gpiod_get(&client->dev, "enp", GPIOD_ASIS);
	if (IS_ERR(bias->enp))
		return dev_err_probe(&client->dev, PTR_ERR(bias->enp),
				     "cannot get first rail GPIO\n");
	bias->enn = devm_gpiod_get(&client->dev, "enn", GPIOD_ASIS);
	if (IS_ERR(bias->enn))
		return dev_err_probe(&client->dev, PTR_ERR(bias->enn),
				     "cannot get second rail GPIO\n");
	bias->reset = devm_gpiod_get(&client->dev, "reset", GPIOD_ASIS);
	if (IS_ERR(bias->reset))
		return dev_err_probe(&client->dev, PTR_ERR(bias->reset),
				     "cannot get panel reset GPIO\n");

	config.driver_data = bias;
	rdev = devm_regulator_register(&client->dev, &cp3nv1_bias_desc, &config);
	if (IS_ERR(rdev))
		return dev_err_probe(&client->dev, PTR_ERR(rdev),
				     "cannot register LCD sequencing supply\n");
	return 0;
}

/* Deliberately do not bind arbitrary SM5109 boards to this fixed sequence. */
static const struct of_device_id cp3nv1_bias_of_match[] = {
	{ .compatible = "coolpad,cp3nv1-lcd-bias" },
	{ }
};
MODULE_DEVICE_TABLE(of, cp3nv1_bias_of_match);

static struct i2c_driver cp3nv1_bias_driver = {
	.driver = {
		.name = "cp3nv1-lcd-bias",
		.of_match_table = cp3nv1_bias_of_match,
	},
	.probe = cp3nv1_bias_probe,
};
module_i2c_driver(cp3nv1_bias_driver);

MODULE_DESCRIPTION("CP3NV1 LCD bias and reset sequencing supply");
MODULE_LICENSE("GPL");

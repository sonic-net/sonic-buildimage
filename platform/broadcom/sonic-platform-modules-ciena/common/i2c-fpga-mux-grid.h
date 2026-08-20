#include <linux/kobject.h>

struct i2c_fpga_mux_grid {
	struct ciena_i2c_err_state *err_state;
	struct kobject 		    err_kobj;
	struct kobject 		    sda_kobj;
	struct kobject 		    scl_kobj;
};

/* ------------------------------------------------------------------------- */
static ssize_t mux_grid_status_show(struct kobject *kobj,
				    struct kobj_attribute *attrs,
				    char *buf)
{
	struct i2c_fpga_mux_grid *grid;
	const char               *status = "UNKNOWN";

	grid = container_of(kobj, struct i2c_fpga_mux_grid, err_kobj);

	if (grid->err_state->prev_error) status = "FAIL";
	else if (grid->err_state->since) status = "OK";

	return sprintf(buf, "%s\n", status);
}
static struct kobj_attribute mux_grid_status_attribute =
	__ATTR(status, 0444, mux_grid_status_show, NULL);

static ssize_t mux_grid_period_show(time64_t tstamp,
				    char    *buf)
{
	time64_t period = -1;

	if (tstamp) period = ktime_get_seconds() - tstamp;

	return sprintf(buf, "%lld\n", period);

}

static ssize_t mux_grid_since_show(struct kobject *kobj,
				   struct kobj_attribute *attrs,
				   char *buf)
{
	struct i2c_fpga_mux_grid *grid;

	grid = container_of(kobj, struct i2c_fpga_mux_grid, err_kobj);

	return mux_grid_period_show(grid->err_state->since, buf);
}
static struct kobj_attribute mux_grid_since_attribute =
	__ATTR(since, 0444, mux_grid_since_show, NULL);

static ssize_t mux_grid_ok_cnt_show(struct kobject *kobj,
				    struct kobj_attribute *attrs,
				    char *buf)
{
	struct i2c_fpga_mux_grid *grid;

	grid = container_of(kobj, struct i2c_fpga_mux_grid, err_kobj);

	return sprintf(buf, "%u\n", grid->err_state->ok_count);
}
static struct kobj_attribute mux_grid_ok_cnt_attribute =
	__ATTR(ok_cnt, 0444, mux_grid_ok_cnt_show, NULL);

static ssize_t mux_grid_last_rc_show(struct kobject *kobj,
				     struct kobj_attribute *attrs,
				     char *buf)
{
	struct i2c_fpga_mux_grid *grid;

	grid = container_of(kobj, struct i2c_fpga_mux_grid, err_kobj);

	return sprintf(buf, "%d\n", grid->err_state->prev_error);
}
static struct kobj_attribute mux_grid_last_rc_attribute =
	__ATTR(last_rc, 0444, mux_grid_last_rc_show, NULL);

static ssize_t mux_grid_last_ok_show(struct kobject *kobj,
				     struct kobj_attribute *attrs,
				     char *buf)
{
	struct i2c_fpga_mux_grid *grid;

	grid = container_of(kobj, struct i2c_fpga_mux_grid, err_kobj);

	return mux_grid_period_show(grid->err_state->last_ok, buf);
}
static struct kobj_attribute mux_grid_last_ok_attribute =
	__ATTR(last_ok, 0444, mux_grid_last_ok_show, NULL);

static ssize_t mux_grid_last_fail_show(struct kobject *kobj,
				       struct kobj_attribute *attrs,
				       char *buf)
{
	struct i2c_fpga_mux_grid *grid;

	grid = container_of(kobj, struct i2c_fpga_mux_grid, err_kobj);

	return mux_grid_period_show(grid->err_state->last_err, buf);
}
static struct kobj_attribute mux_grid_last_fail_attribute =
	__ATTR(last_fail, 0444, mux_grid_last_fail_show, NULL);

static ssize_t mux_grid_err_cnt_show(struct kobject *kobj,
				     struct kobj_attribute *attrs,
				     char *buf)
{
	struct i2c_fpga_mux_grid *grid;

	grid = container_of(kobj, struct i2c_fpga_mux_grid, err_kobj);

	return sprintf(buf, "%u\n", grid->err_state->err_count);
}
static struct kobj_attribute mux_grid_err_cnt_attribute =
	__ATTR(err_cnt, 0444, mux_grid_err_cnt_show, NULL);

static const struct attribute *mux_grid_attrs[] = {
	&mux_grid_status_attribute.attr,
	&mux_grid_since_attribute.attr,
	&mux_grid_ok_cnt_attribute.attr,
	&mux_grid_last_rc_attribute.attr,
	&mux_grid_last_ok_attribute.attr,
	&mux_grid_last_fail_attribute.attr,
	&mux_grid_err_cnt_attribute.attr,
	NULL,
};

static struct kobj_type mux_grid_ktype = {
	.sysfs_ops = &kobj_sysfs_ops,
};

/* ------------------------------------------------------------------------- */
static void mux_destroy_error_subdirs(struct i2c_fpga_mux_priv *priv,
				      struct i2c_fpga_mux_grid *grid)
{
	if (priv->scl_mask) {
		kobject_del(&grid->scl_kobj);
		kobject_put(&grid->scl_kobj);
	}

	if (priv->sda_mask) {
		kobject_del(&grid->sda_kobj);
		kobject_put(&grid->sda_kobj);
	}

	sysfs_remove_files(&grid->err_kobj, mux_grid_attrs);
	kobject_del(&grid->err_kobj);
	kobject_put(&grid->err_kobj);
}

/* ------------------------------------------------------------------------- */
static int mux_do_shared_links(struct i2c_fpga_mux_priv *priv,
			       struct kobject           *parent_kobj,
			       int                       mux_index,
			       u32                       value_mask,
			       int                       max,
			       bool                      create)
{
	struct i2c_adapter *parent_adap;
	struct i2c_adapter *adap;
	unsigned            mux_val;
	int                 rc = 0;
	int                 i;

	if (!value_mask) return 0;

	parent_adap = priv->mux_core->adapter[mux_index];
	mux_val     = priv->values[mux_index] & value_mask;

	for (i = 0; i < max; i++) {
		/* do not link an adapter back to itself */
		if (mux_index == i) continue;

		adap = priv->mux_core->adapter[i];
		if (mux_val != (priv->values[i] & value_mask)) continue;

		if (create) {
			rc = sysfs_create_link(parent_kobj, &adap->dev.kobj,
					       dev_name(&adap->dev));
			if (rc) {
				dev_err(&parent_adap->dev,
					"cannot link to %s (%d)\n",
					dev_name(&adap->dev), rc);
				goto out_unlink;
			}
		}
		else sysfs_remove_link(parent_kobj, dev_name(&adap->dev));
	}
	return 0;

out_unlink:
	mux_do_shared_links(priv, parent_kobj, mux_index,
			    value_mask, i, false);
	return rc;
}

/* ------------------------------------------------------------------------- */
static void mux_depopulate_error_grid(struct i2c_fpga_mux_priv *priv,
				      int                       max)
{
	int i;

	if (!priv->grid) return;

	for (i = 0; i < max; i++) {
		mux_do_shared_links(priv, &priv->grid[i].sda_kobj, i,
				    priv->sda_mask, priv->n_adap, false);

		mux_do_shared_links(priv, &priv->grid[i].scl_kobj, i,
				    priv->scl_mask, priv->n_adap, false);

		mux_destroy_error_subdirs(priv, &priv->grid[i]);
	}
}

/* ------------------------------------------------------------------------- */
static int mux_create_error_subdirs(struct i2c_fpga_mux_priv *priv,
				    struct i2c_fpga_mux_grid *grid,
				    struct i2c_adapter       *adap)
{
	int rc;

	rc = kobject_init_and_add(&grid->err_kobj, &mux_grid_ktype,
				  &adap->dev.kobj, "error_state");
	if (rc) {
		dev_err(&adap->dev, "cannot create error_state (%d)\n", rc);
		return rc;
	}

	if (priv->sda_mask) {
		rc = kobject_init_and_add(&grid->sda_kobj, &mux_grid_ktype,
					  &grid->err_kobj, "shared_sda");

		if (rc) {
			dev_err(&adap->dev, "add shared_sda failed(%d)\n", rc);
			goto put_err;
		}
	}

	if (priv->scl_mask) {
		rc = kobject_init_and_add(&grid->scl_kobj, &mux_grid_ktype,
					  &grid->err_kobj, "shared_scl");
		if (rc) {
			dev_err(&adap->dev, "add shared_scl failed(%d)\n", rc);
			goto put_sda;
		}
	}

	rc = sysfs_create_files(&grid->err_kobj, mux_grid_attrs);
	if (rc) {
		dev_err(&adap->dev, "cannot create files (%d)\n", rc);
		goto put_scl;
	}

	return 0;

put_scl:
	if (priv->scl_mask) {
		kobject_del(&grid->scl_kobj);
		kobject_put(&grid->scl_kobj);
	}
put_sda:
	if (priv->sda_mask) {
		kobject_del(&grid->sda_kobj);
		kobject_put(&grid->sda_kobj);
	}
put_err:
	kobject_del(&grid->err_kobj);
	kobject_put(&grid->err_kobj);

	return rc;
}

/* ------------------------------------------------------------------------- */
static int mux_populate_error_grid(struct i2c_fpga_mux_priv *priv)
{
	struct i2c_fpga_mux_grid *grid;
	struct i2c_adapter       *adap;
	int                       rc;
	int                       i;

	if (!priv->children_err) {
		dev_err(&priv->pdev->dev, "grid requires error tracking\n");
		return -EINVAL;
	}

	priv->grid = devm_kzalloc(&priv->pdev->dev,
				  priv->n_adap * sizeof(*priv->grid),
				  GFP_KERNEL);
	if (!priv->grid) {
		dev_err(&priv->pdev->dev, "cannot allocate grid array\n");
		return -ENOMEM;
	}

	for (i = 0; i < priv->n_adap; i++) {
		adap = priv->mux_core->adapter[i];
		grid = &priv->grid[i];

		grid->err_state = &priv->children_err[i];

		rc = mux_create_error_subdirs(priv, grid, adap);
		if (rc) break;

		rc = mux_do_shared_links(priv, &grid->sda_kobj, i,
					 priv->sda_mask, priv->n_adap, true);
		if (rc) {
			mux_destroy_error_subdirs(priv, grid);
			break;
		}

		rc = mux_do_shared_links(priv, &grid->scl_kobj, i,
					 priv->scl_mask, priv->n_adap, true);
		if (rc) {
			mux_do_shared_links(priv, &grid->sda_kobj, i,
					    priv->sda_mask, priv->n_adap,
					    false);
			mux_destroy_error_subdirs(priv, grid);
			break;
		}

	}

	if (rc) {
		mux_depopulate_error_grid(priv, i);
		return rc;
	}

	return 0;
}

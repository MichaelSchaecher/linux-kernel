/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_GENHD_H
#define _LINUX_GENHD_H

/*
 * 	genhd.h Copyright (C) 1992 Drew Eckhardt
 *	Generic hard disk header file by  
 * 		Drew Eckhardt
 *
 *		<drew@colorado.edu>
 */

#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/uuid.h>
#include <linux/blk_types.h>
#include <linux/device.h>
#include <linux/xarray.h>

extern const struct device_type disk_type;
extern struct device_type part_type;
extern struct class block_class;

#define DISK_MAX_PARTS			256
#define DISK_NAME_LEN			32

#define PARTITION_META_INFO_VOLNAMELTH	64
/*
 * Enough for the string representation of any kind of UUID plus NULL.
 * EFI UUID is 36 characters. MSDOS UUID is 11 characters.
 */
#define PARTITION_META_INFO_UUIDLTH	(UUID_STRING_LEN + 1)

struct partition_meta_info {
	char uuid[PARTITION_META_INFO_UUIDLTH];
	u8 volname[PARTITION_META_INFO_VOLNAMELTH];
};

/**
 * DOC: genhd capability flags
 *
 * ``GENHD_FL_REMOVABLE``: indicates that the block device gives access to
 * removable media.  When set, the device remains present even when media is not
 * inserted.  Shall not be set for devices which are removed entirely when the
 * media is removed.
 *
 * ``GENHD_FL_HIDDEN``: the block device is hidden; it doesn't produce events,
 * doesn't appear in sysfs, and can't be opened from userspace or using
 * blkdev_get*. Used for the underlying components of multipath devices.
 *
 * ``GENHD_FL_NO_PART``: partition support is disabled.  The kernel will not
 * scan for partitions from add_disk, and users can't add partitions manually.
 *
 */
enum {
	GENHD_FL_REMOVABLE			= 1 << 0,
	GENHD_FL_HIDDEN				= 1 << 1,
	GENHD_FL_NO_PART			= 1 << 2,
};

enum {
	DISK_EVENT_MEDIA_CHANGE			= 1 << 0, /* media changed */
	DISK_EVENT_EJECT_REQUEST		= 1 << 1, /* eject requested */
};

enum {
	/* Poll even if events_poll_msecs is unset */
	DISK_EVENT_FLAG_POLL			= 1 << 0,
	/* Forward events to udev */
	DISK_EVENT_FLAG_UEVENT			= 1 << 1,
	/* Block event polling when open for exclusive write */
	DISK_EVENT_FLAG_BLOCK_ON_EXCL_WRITE	= 1 << 2,
};

struct disk_events;
struct badblocks;

struct blk_integrity {
	const struct blk_integrity_profile	*profile;
	unsigned char				flags;
	unsigned char				tuple_size;
	unsigned char				interval_exp;
	unsigned char				tag_size;
};

struct gendisk {
	/*
	 * major/first_minor/minors should not be set by any new driver, the
	 * block core will take care of allocating them automatically.
	 */
	int major;
	int first_minor;
	int minors;

	char disk_name[DISK_NAME_LEN];	/* name of major driver */

	unsigned short events;		/* supported events */
	unsigned short event_flags;	/* flags related to event processing */

	struct xarray part_tbl;
	struct block_device *part0;

	const struct block_device_operations *fops;
	struct request_queue *queue;
	void *private_data;

	int flags;
	unsigned long state;
#define GD_NEED_PART_SCAN		0
#define GD_READ_ONLY			1
#define GD_DEAD				2
#define GD_NATIVE_CAPACITY		3
#define GD_SUPPRESS_PART_SCAN		5

	struct mutex open_mutex;	/* open/close mutex */
	unsigned open_partitions;	/* number of open partitions */

	struct backing_dev_info	*bdi;
	struct kobject *slave_dir;
#ifdef CONFIG_BLOCK_HOLDER_DEPRECATED
	struct list_head slave_bdevs;
#endif
	struct timer_rand_state *random;
	atomic_t sync_io;		/* RAID */
	struct disk_events *ev;
#ifdef  CONFIG_BLK_DEV_INTEGRITY
	struct kobject integrity_kobj;
#endif	/* CONFIG_BLK_DEV_INTEGRITY */
#if IS_ENABLED(CONFIG_CDROM)
	struct cdrom_device_info *cdi;
#endif
	int node_id;
	struct badblocks *bb;
	struct lockdep_map lockdep_map;
	u64 diskseq;
};

static inline bool disk_live(struct gendisk *disk)
{
	return !inode_unhashed(disk->part0->bd_inode);
}

/*
 * The gendisk is refcounted by the part0 block_device, and the bd_device
 * therein is also used for device model presentation in sysfs.
 */
#define dev_to_disk(device) \
	(dev_to_bdev(device)->bd_disk)
#define disk_to_dev(disk) \
	(&((disk)->part0->bd_device))

#if IS_REACHABLE(CONFIG_CDROM)
#define disk_to_cdi(disk)	((disk)->cdi)
#else
#define disk_to_cdi(disk)	NULL
#endif

static inline dev_t disk_devt(struct gendisk *disk)
{
	return MKDEV(disk->major, disk->first_minor);
}

void disk_uevent(struct gendisk *disk, enum kobject_action action);

/* block/genhd.c */
int __must_check device_add_disk(struct device *parent, struct gendisk *disk,
				 const struct attribute_group **groups);
static inline int __must_check add_disk(struct gendisk *disk)
{
	return device_add_disk(NULL, disk, NULL);
}
extern void del_gendisk(struct gendisk *gp);

void invalidate_disk(struct gendisk *disk);

void set_disk_ro(struct gendisk *disk, bool read_only);

static inline int get_disk_ro(struct gendisk *disk)
{
	return disk->part0->bd_read_only ||
		test_bit(GD_READ_ONLY, &disk->state);
}

static inline int bdev_read_only(struct block_device *bdev)
{
	return bdev->bd_read_only || get_disk_ro(bdev->bd_disk);
}

extern void disk_block_events(struct gendisk *disk);
extern void disk_unblock_events(struct gendisk *disk);
extern void disk_flush_events(struct gendisk *disk, unsigned int mask);
bool set_capacity_and_notify(struct gendisk *disk, sector_t size);
bool disk_force_media_change(struct gendisk *disk, unsigned int events);

/* drivers/char/random.c */
extern void add_disk_randomness(struct gendisk *disk) __latent_entropy;
extern void rand_initialize_disk(struct gendisk *disk);

static inline sector_t get_start_sect(struct block_device *bdev)
{
	return bdev->bd_start_sect;
}

static inline sector_t bdev_nr_sectors(struct block_device *bdev)
{
	return bdev->bd_nr_sectors;
}

static inline loff_t bdev_nr_bytes(struct block_device *bdev)
{
	return (loff_t)bdev_nr_sectors(bdev) << SECTOR_SHIFT;
}

static inline sector_t get_capacity(struct gendisk *disk)
{
	return bdev_nr_sectors(disk->part0);
}

static inline u64 sb_bdev_nr_blocks(struct super_block *sb)
{
	return bdev_nr_sectors(sb->s_bdev) >>
		(sb->s_blocksize_bits - SECTOR_SHIFT);
}

int bdev_disk_changed(struct gendisk *disk, bool invalidate);
void blk_drop_partitions(struct gendisk *disk);

struct gendisk *__alloc_disk_node(struct request_queue *q, int node_id,
		struct lock_class_key *lkclass);
extern void put_disk(struct gendisk *disk);
struct gendisk *__blk_alloc_disk(int node, struct lock_class_key *lkclass);

/**
 * blk_alloc_disk - allocate a gendisk structure
 * @node_id: numa node to allocate on
 *
 * Allocate and pre-initialize a gendisk structure for use with BIO based
 * drivers.
 *
 * Context: can sleep
 */
#define blk_alloc_disk(node_id)						\
({									\
	static struct lock_class_key __key;				\
									\
	__blk_alloc_disk(node_id, &__key);				\
})
void blk_cleanup_disk(struct gendisk *disk);

int __register_blkdev(unsigned int major, const char *name,
		void (*probe)(dev_t devt));
#define register_blkdev(major, name) \
	__register_blkdev(major, name, NULL)
void unregister_blkdev(unsigned int major, const char *name);

bool bdev_check_media_change(struct block_device *bdev);
int __invalidate_device(struct block_device *bdev, bool kill_dirty);
void set_capacity(struct gendisk *disk, sector_t size);

#ifdef CONFIG_BLOCK_HOLDER_DEPRECATED
int bd_link_disk_holder(struct block_device *bdev, struct gendisk *disk);
void bd_unlink_disk_holder(struct block_device *bdev, struct gendisk *disk);
int bd_register_pending_holders(struct gendisk *disk);
#else
static inline int bd_link_disk_holder(struct block_device *bdev,
				      struct gendisk *disk)
{
	return 0;
}
static inline void bd_unlink_disk_holder(struct block_device *bdev,
					 struct gendisk *disk)
{
}
static inline int bd_register_pending_holders(struct gendisk *disk)
{
	return 0;
}
#endif /* CONFIG_BLOCK_HOLDER_DEPRECATED */

dev_t part_devt(struct gendisk *disk, u8 partno);
void inc_diskseq(struct gendisk *disk);
dev_t blk_lookup_devt(const char *name, int partno);
void blk_request_module(dev_t devt);
#ifdef CONFIG_BLOCK
void printk_all_partitions(void);
#else /* CONFIG_BLOCK */
static inline void printk_all_partitions(void)
{
}
#endif /* CONFIG_BLOCK */

#endif /* _LINUX_GENHD_H */
